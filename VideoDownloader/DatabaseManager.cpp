#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <memory>
#include <algorithm>

#include "LogSystem.h"

// 常量定义
const QString DatabaseManager::DEFAULT_DATABASE_NAME = "VideoDownloader.db";
const QString DatabaseManager::CONNECTION_NAME = "VideoDownloader_main_connection";
const int DatabaseManager::DEFAULT_QUERY_TIMEOUT = 30000; // 30秒
const int DatabaseManager::DEFAULT_RETRY_COUNT = 3;
const int DatabaseManager::PREPARED_QUERY_CLEANUP_INTERVAL = 60000; // 1分钟
const int DatabaseManager::DEFAULT_MAX_PREPARED_QUERIES = 50; // 默认最大预编译查询数量

DatabaseManager* DatabaseManager::m_instance = nullptr;
QMutex DatabaseManager::m_mutex;

// ScopedTransaction 实现
DatabaseManager::ScopedTransaction::ScopedTransaction(DatabaseManager* dbManager)
	: m_dbManager(dbManager)
	, m_started(false)
	, m_committed(false)
{
	if (m_dbManager) {
		m_started = m_dbManager->beginTransaction();
	}
}

DatabaseManager::ScopedTransaction::~ScopedTransaction()
{
	if (m_started && !m_committed) {
		m_dbManager->rollbackTransaction();
	}
}

bool DatabaseManager::ScopedTransaction::commit()
{
	if (m_started && !m_committed) {
		m_committed = m_dbManager->commitTransaction();
		return m_committed;
	}
	return false;
}

bool DatabaseManager::ScopedTransaction::isActive() const
{
	return m_started && !m_committed;
}

// DatabaseManager 实现
DatabaseManager::DatabaseManager(QObject* parent)
	: QObject(parent)
	, m_queryTimeout(DEFAULT_QUERY_TIMEOUT)
	, m_retryCount(DEFAULT_RETRY_COUNT)
	, m_maxPreparedQueries(DEFAULT_MAX_PREPARED_QUERIES)
{
	m_database = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);

	// 设置预编译查询清理定时器
	m_preparedQueryCleanupTimer = new QTimer(this);
	connect(m_preparedQueryCleanupTimer, &QTimer::timeout,
		this, &DatabaseManager::cleanupUnusedPreparedQueriesSlot);
	m_preparedQueryCleanupTimer->start(PREPARED_QUERY_CLEANUP_INTERVAL);
}

DatabaseManager::~DatabaseManager()
{
	// 安全地停止和删除定时器
	if (m_preparedQueryCleanupTimer)
	{
		if (m_preparedQueryCleanupTimer->isActive()) {
			m_preparedQueryCleanupTimer->stop();
		}
		delete m_preparedQueryCleanupTimer;
		m_preparedQueryCleanupTimer = nullptr;
	}

	// 清理预编译查询
	clearPreparedQueries();

	// 关闭数据库
	closeDatabase();

	// 安全地移除数据库连接
	if (QSqlDatabase::contains(CONNECTION_NAME))
	{
		QSqlDatabase::removeDatabase(CONNECTION_NAME);
	}
}

DatabaseManager* DatabaseManager::instance()
{
	QMutexLocker locker(&m_mutex);
	if (!m_instance)
		m_instance = new DatabaseManager();
	return m_instance;
}

bool DatabaseManager::openDatabase(const QString& databaseName)
{
	// 检查数据库驱动是否可用
	if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
	{
		LOG_ERROR("数据库驱动不可用", "QSQLITE驱动未加载");
		return false;
	}

	// 获取应用数据目录
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dir(dataPath);
	if (!dir.exists())
	{
		if (!dir.mkpath("."))
		{
			LOG_ERROR("创建数据目录失败", dataPath);
			return false;
		}
	}

	QString dbPath = dir.filePath(databaseName.isEmpty() ? DEFAULT_DATABASE_NAME : databaseName);
	m_database.setDatabaseName(dbPath);

	if (!m_database.open())
	{
		LOG_ERROR("数据库打开失败", QString("路径: %1, 错误: %2").arg(dbPath, m_database.lastError().text()));
		return false;
	}

	// 数据库初始化
	if (!initializeDatabase())
	{
		LOG_ERROR("数据库初始化失败", lastError());
		closeDatabase();
		return false;
	}

	return true;
}

bool DatabaseManager::initializeDatabase()
{
	// 启用外键约束
	if (!executeQuery("PRAGMA foreign_keys = ON"))
	{
		LOG_ERROR("外键约束启用失败", lastError());
		return false;
	}

	// 设置WAL模式提高并发性能
	if (!executeQuery("PRAGMA journal_mode = WAL"))
	{
		LOG_ERROR("WAL模式设置失败", lastError());
		// 不是致命错误，继续执行
	}

	if (!executeQuery("PRAGMA synchronous = NORMAL"))
	{
		LOG_ERROR("同步模式设置失败", lastError());
		// 不是致命错误，继续执行
	}

	// 设置缓存大小
	if (!executeQuery("PRAGMA cache_size = -64000")) // 64MB
	{
		LOG_ERROR("缓存大小设置失败", lastError());
		// 不是致命错误，继续执行
	}

	// 设置繁忙超时
	if (!executeQuery("PRAGMA busy_timeout = 5000")) // 5秒
	{
		LOG_ERROR("设置繁忙超时失败", lastError());
		// 不是致命错误，继续执行
	}

	return true;
}

void DatabaseManager::closeDatabase()
{
	if (m_database.isOpen())
		m_database.close();
}

bool DatabaseManager::isOpen() const
{
	return m_database.isOpen();
}

bool DatabaseManager::checkConnectionHealth()
{
	if (!m_database.isOpen())
		return false;

	QSqlQuery query(m_database);
	return query.exec("SELECT 1");
}

bool DatabaseManager::reconnect()
{
	QString dbPath = m_database.databaseName();
	closeDatabase();
	return openDatabase(dbPath);
}

bool DatabaseManager::createTable(const QString& tableName, const QString& tableDefinition)
{
	if (tableName.isEmpty() || tableDefinition.isEmpty())
	{
		LOG_ERROR("创建表失败", "表名或表定义为空");
		return false;
	}

	QString query = QString("CREATE TABLE IF NOT EXISTS %1 (%2)").arg(tableName, tableDefinition);
	return executeQueryWithRetry(query);
}

bool DatabaseManager::dropTable(const QString& tableName)
{
	return executeQueryWithRetry(QString("DROP TABLE IF EXISTS %1").arg(tableName));
}

bool DatabaseManager::tableExists(const QString& tableName)
{
	if (!m_database.isOpen())
		return false;

	bool exists = false;
	QString query = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
	executeSelect(query, { tableName }, [&](QSqlQuery& result) {
		exists = result.next() && (result.value(0).toString() == tableName);
		});

	return exists;
}

bool DatabaseManager::truncateTable(const QString& tableName)
{
	if (!executeQueryWithRetry(QString("DELETE FROM %1").arg(tableName)))
		return false;

	return executeQueryWithRetry(QString("VACUUM"));
}

bool DatabaseManager::beginTransaction()
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("开始事务失败", "数据库未打开");
		return false;
	}

	if (m_database.transaction())
	{
		LOG_ERROR("开始事务失败", "已经在事务中");
		return false;
	}

	return executeQueryWithRetry("BEGIN TRANSACTION");
}

bool DatabaseManager::commitTransaction()
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("提交事务失败", "数据库未打开");
		return false;
	}
	return executeQueryWithRetry("COMMIT");
}

bool DatabaseManager::rollbackTransaction()
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("回滚事务失败", "数据库未打开");
		return false;
	}
	return executeQueryWithRetry("ROLLBACK");
}

bool DatabaseManager::executeQuery(const QString& query, const QVariantList& params)
{
	return executeQueryWithRetry(query, params);
}

bool DatabaseManager::shouldRetry(const QSqlError& error) const
{
	QString errorText = error.text();
	return errorText.contains("locked", Qt::CaseInsensitive) ||
		errorText.contains("busy", Qt::CaseInsensitive) ||
		errorText.contains("timeout", Qt::CaseInsensitive);
}

int DatabaseManager::calculateBackoff(int attempt) const
{
	return qMin(1000, 100 * (1 << (attempt - 1)));
}

bool DatabaseManager::executeQueryWithRetry(const QString& query, const QVariantList& params)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("执行SQL失败", "数据库未打开");
		return false;
	}

	int attempts = 0;
	while (attempts <= m_retryCount)
	{
		QSqlQuery sqlQuery(m_database);
		if (executeQueryInternal(sqlQuery, query, params))
		{
			return true;
		}

		if (shouldRetry(sqlQuery.lastError()))
		{
			attempts++;
			if (attempts <= m_retryCount)
			{
				int backoffMs = calculateBackoff(attempts);
				LOG_WARN("数据库繁忙，重试中", QString("尝试 %1/%2, 查询: %3, 退避: %4ms")
					.arg(attempts).arg(m_retryCount).arg(query).arg(backoffMs));
				QThread::msleep(backoffMs);
				continue;
			}
		}

		break;
	}

	QString error = lastError();
	LOG_ERROR("SQL执行失败，已达到最大重试次数", QString("查询: %1, 错误: %2").arg(query, error));
	return false;
}

bool DatabaseManager::executeQueryInternal(QSqlQuery& sqlQuery, const QString& query, const QVariantList& params)
{
	if (!sqlQuery.prepare(query))
	{
		LOG_ERROR("SQL准备失败", QString("Query: %1, Error: %2").arg(query, sqlQuery.lastError().text()));
		return false;
	}

	for (int i = 0; i < params.size(); ++i)
		sqlQuery.bindValue(i, params[i]);

	if (!sqlQuery.exec())
	{
		LOG_ERROR("SQL执行错误", QString("Query: %1, Error: %2").arg(query, sqlQuery.lastError().text()));
		return false;
	}

	return true;
}

bool DatabaseManager::executeSelect(const QString& query, const QVariantList& params, std::function<void(QSqlQuery&)> resultProcessor)
{
	return executeSelectInternal(query, params, resultProcessor);
}

bool DatabaseManager::executeSelectInternal(const QString& query,
	const QVariantList& params,
	std::function<void(QSqlQuery&)> resultProcessor)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("执行查询失败", "数据库未打开");
		return false;
	}

	QSqlQuery sqlQuery(m_database);
	sqlQuery.setForwardOnly(true);

	if (!sqlQuery.prepare(query))
	{
		LOG_ERROR("SQL准备失败", QString("Query: %1, Error: %2").arg(query, sqlQuery.lastError().text()));
		return false;
	}

	for (int i = 0; i < params.size(); ++i)
		sqlQuery.bindValue(i, params[i]);

	if (!sqlQuery.exec())
	{
		LOG_ERROR("SQL查询错误", QString("Query: %1, Error: %2").arg(query, sqlQuery.lastError().text()));
		return false;
	}

	if (resultProcessor)
	{
		resultProcessor(sqlQuery);
	}

	return true;
}

bool DatabaseManager::executeBatchQuery(const QString& query, const QList<QVariantList>& batchParams)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("执行批量SQL失败", "数据库未打开");
		return false;
	}

	ScopedTransaction transaction(this);
	if (!transaction.isActive())
		return false;

	for (const QVariantList& params : batchParams)
	{
		if (!executeQueryWithRetry(query, params))
		{
			return false;
		}
	}

	return transaction.commit();
}

bool DatabaseManager::prepareQuery(const QString& queryName, const QString& query)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("预编译查询失败", "数据库未打开");
		return false;
	}

	QMutexLocker locker(&m_preparedQueriesMutex);

	// 检查是否已存在同名查询
	if (m_preparedQueries.contains(queryName))
	{
		// 更新已存在的查询
		auto preparedQuery = m_preparedQueries[queryName];
		if (!preparedQuery->query.prepare(query))
		{
			LOG_ERROR("预编译查询更新失败", QString("Query: %1, Error: %2").arg(query, preparedQuery->query.lastError().text()));
			return false;
		}
		preparedQuery->lastUsed = QDateTime::currentMSecsSinceEpoch();
		return true;
	}

	// 检查是否超过最大数量，如果超过则先清理
	if (m_preparedQueries.size() >= m_maxPreparedQueries)
	{
		cleanupUnusedPreparedQueries();

		// 如果清理后仍然超过限制，则移除最久未使用的一个
		if (m_preparedQueries.size() >= m_maxPreparedQueries)
		{
			QString oldestKey;
			qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();

			for (auto it = m_preparedQueries.begin(); it != m_preparedQueries.end(); ++it)
			{
				if (it.value()->lastUsed < oldestTime)
				{
					oldestTime = it.value()->lastUsed;
					oldestKey = it.key();
				}
			}

			if (!oldestKey.isEmpty())
			{
				m_preparedQueries.remove(oldestKey);
				LOG_DEBUG("移除最久未使用的预编译查询", QString("查询: %1").arg(oldestKey));
			}
		}
	}

	auto preparedQuery = std::make_shared<PreparedQuery>();

	if (!preparedQuery->query.prepare(query))
	{
		LOG_ERROR("预编译查询失败", QString("Query: %1, Error: %2").arg(query, preparedQuery->query.lastError().text()));
		return false;
	}

	preparedQuery->lastUsed = QDateTime::currentMSecsSinceEpoch();
	m_preparedQueries[queryName] = preparedQuery;

	LOG_DEBUG("预编译查询已准备", QString("查询: %1, 当前总数: %2").arg(queryName).arg(m_preparedQueries.size()));
	return true;
}

bool DatabaseManager::executePreparedQuery(const QString& queryName, const QVariantList& params)
{
	QMutexLocker locker(&m_preparedQueriesMutex);

	if (!m_preparedQueries.contains(queryName))
	{
		LOG_ERROR("执行预编译查询失败", "查询未预编译: " + queryName);
		return false;
	}

	auto preparedQuery = m_preparedQueries[queryName];
	preparedQuery->lastUsed = QDateTime::currentMSecsSinceEpoch();

	QSqlQuery& sqlQuery = preparedQuery->query;
	sqlQuery.finish();

	for (int i = 0; i < params.size(); ++i)
		sqlQuery.bindValue(i, params[i]);

	if (!sqlQuery.exec())
	{
		LOG_ERROR("预编译查询执行失败",
			QString("Query: %1, Error: %2")
			.arg(queryName, sqlQuery.lastError().text()));
		return false;
	}

	return true;
}

bool DatabaseManager::executePreparedSelect(const QString& queryName,
	const QVariantList& params,
	std::function<void(QSqlQuery&)> resultProcessor)
{
	QMutexLocker locker(&m_preparedQueriesMutex);

	if (!m_preparedQueries.contains(queryName))
	{
		LOG_ERROR("执行预编译查询失败", "查询未预编译: " + queryName);
		return false;
	}

	auto preparedQuery = m_preparedQueries[queryName];
	preparedQuery->lastUsed = QDateTime::currentMSecsSinceEpoch();

	QSqlQuery& sqlQuery = preparedQuery->query;
	sqlQuery.finish();

	for (int i = 0; i < params.size(); ++i)
		sqlQuery.bindValue(i, params[i]);

	if (!sqlQuery.exec())
	{
		LOG_ERROR("预编译SQL查询错误", QString("Query: %1, Error: %2").arg(queryName, sqlQuery.lastError().text()));
		return false;
	}

	if (resultProcessor)
	{
		try {
			resultProcessor(sqlQuery);
		}
		catch (...) {
			LOG_ERROR("预编译查询结果处理异常", "查询: " + queryName);
			return false;
		}
	}

	return true;
}

void DatabaseManager::clearPreparedQueries()
{
	QMutexLocker locker(&m_preparedQueriesMutex);
	int count = m_preparedQueries.size();
	m_preparedQueries.clear();
	LOG_DEBUG("清理所有预编译查询", QString("共清理 %1 个查询").arg(count));
}

void DatabaseManager::cleanupUnusedPreparedQueries()
{
	QMutexLocker locker(&m_preparedQueriesMutex);

	int currentCount = m_preparedQueries.size();

	// 只有当数量超过最大值时才启动清理
	if (currentCount <= m_maxPreparedQueries)
		return;

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

	// 创建按最后使用时间排序的列表
	QList<QPair<QString, qint64>> queryUsage;
	for (auto it = m_preparedQueries.begin(); it != m_preparedQueries.end(); ++it)
		queryUsage.append(qMakePair(it.key(), it.value()->lastUsed));

	// 按最后使用时间升序排序（最久未使用的在前面）
	std::sort(queryUsage.begin(), queryUsage.end(),
		[](const QPair<QString, qint64>& a, const QPair<QString, qint64>& b) {
			return a.second < b.second;
		});

	// 计算需要清理的数量
	int queriesToRemove = currentCount - m_maxPreparedQueries;
	QStringList keysToRemove;

	// 只清理最久未使用的查询，直到达到目标数量
	for (int i = 0; i < queriesToRemove && i < queryUsage.size(); ++i)
		keysToRemove.append(queryUsage[i].first);

	// 执行清理
	for (const QString& key : keysToRemove)
		m_preparedQueries.remove(key);

	if (!keysToRemove.isEmpty())
	{
		LOG_DEBUG("清理过期预编译查询",
			QString("当前数量: %1, 目标数量: %2, 移除了 %3 个最久未使用的查询")
			.arg(currentCount).arg(m_maxPreparedQueries).arg(keysToRemove.size()));
	}
}

void DatabaseManager::cleanupUnusedPreparedQueriesSlot()
{
	cleanupUnusedPreparedQueries();
}

// 预编译查询配置方法
void DatabaseManager::setMaxPreparedQueries(int maxQueries)
{
	QMutexLocker locker(&m_preparedQueriesMutex);
	if (maxQueries > 0)
	{
		int oldValue = m_maxPreparedQueries;
		m_maxPreparedQueries = maxQueries;

		// 立即检查是否需要清理
		if (m_preparedQueries.size() > m_maxPreparedQueries)
			cleanupUnusedPreparedQueries();

		LOG_DEBUG("设置最大预编译查询数量",
			QString("从 %1 改为 %2").arg(oldValue).arg(m_maxPreparedQueries));
	}
}

int DatabaseManager::maxPreparedQueries() const
{
	return m_maxPreparedQueries;
}

int DatabaseManager::preparedQueryCount() const
{
	QMutexLocker locker(&m_preparedQueriesMutex);
	return m_preparedQueries.size();
}

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
	// 使用SQLite的备份命令
	return backupUsingSqliteBackupCommand(backupPath);
}

bool DatabaseManager::backupUsingSqliteBackupCommand(const QString& backupPath)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("SQLite备份失败", "数据库未打开");
		return false;
	}

	// 使用SQLite的.backup命令进行备份
	// 这个方法不需要直接访问SQLite C API
	QString currentDbPath = m_database.databaseName();
	if (currentDbPath.isEmpty())
	{
		LOG_ERROR("SQLite备份失败", "数据库路径为空");
		return false;
	}

	// 执行备份命令
	QString backupCommand = QString("BACKUP TO '%1'").arg(backupPath);

	// 使用事务确保备份的一致性
	ScopedTransaction transaction(this);
	if (!transaction.isActive())
	{
		LOG_ERROR("SQLite备份事务启动失败", backupPath);
		return false;
	}

	// 执行备份
	if (!executeQuery(backupCommand))
	{
		LOG_ERROR("SQLite备份命令执行失败", QString("命令: %1").arg(backupCommand));
		return false;
	}

	// 提交事务
	if (!transaction.commit())
	{
		LOG_ERROR("SQLite备份事务提交失败", backupPath);
		return false;
	}

	LOG_INFO("SQLite备份成功", QString("备份路径: %1").arg(backupPath));
	return true;
}

bool DatabaseManager::backupUsingFileCopy(const QString& backupPath)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("文件复制备份失败", "数据库未打开");
		return false;
	}

	QString currentDbPath = m_database.databaseName();
	if (currentDbPath.isEmpty())
	{
		LOG_ERROR("文件复制备份失败", "数据库路径为空");
		return false;
	}

	// 确保没有未提交的事务
	commitTransaction();

	// 关闭数据库连接以确保文件没有被锁定
	closeDatabase();

	bool success = false;
	if (QFile::exists(currentDbPath))
	{
		// 删除已存在的备份文件
		if (QFile::exists(backupPath))
			QFile::remove(backupPath);

		success = QFile::copy(currentDbPath, backupPath);
	}

	// 重新打开数据库
	if (!openDatabase(currentDbPath))
	{
		LOG_ERROR("重新打开数据库失败", "备份后无法重新打开数据库");
		return false;
	}

	if (success)
		LOG_INFO("文件复制备份成功", QString("备份路径: %1").arg(backupPath));
	else
		LOG_ERROR("文件复制备份失败", QString("从 %1 到 %2").arg(currentDbPath, backupPath));

	return success;
}

bool DatabaseManager::restoreDatabase(const QString& backupPath)
{
	// 使用SQLite的恢复命令
	return restoreUsingSqliteRestoreCommand(backupPath);
}

bool DatabaseManager::restoreUsingSqliteRestoreCommand(const QString& backupPath)
{
	if (!QFile::exists(backupPath))
	{
		LOG_ERROR("恢复数据库失败", "备份文件不存在: " + backupPath);
		return false;
	}

	QString currentDbPath = m_database.databaseName();
	if (currentDbPath.isEmpty())
	{
		LOG_ERROR("恢复数据库失败", "当前数据库路径为空");
		return false;
	}

	// 关闭当前数据库连接
	closeDatabase();

	// 备份当前数据库
	QString backupCurrent = currentDbPath + ".backup";
	bool hasCurrentBackup = false;
	if (QFile::exists(currentDbPath))
	{
		if (QFile::copy(currentDbPath, backupCurrent))
			hasCurrentBackup = true;
		LOG_WARN("无法备份当前数据库", "继续恢复操作");
	}

	// 删除当前数据库文件
	if (QFile::exists(currentDbPath) && !QFile::remove(currentDbPath))
	{
		LOG_ERROR("删除当前数据库失败", currentDbPath);
		if (hasCurrentBackup)
			QFile::copy(backupCurrent, currentDbPath);

		openDatabase(currentDbPath);
		return false;
	}

	// 重新打开数据库连接
	if (!openDatabase(currentDbPath))
	{
		LOG_ERROR("重新打开数据库失败", "恢复前无法打开数据库");
		if (hasCurrentBackup)
			QFile::copy(backupCurrent, currentDbPath);

		return false;
	}

	// 使用SQLite的.restore命令进行恢复
	QString restoreCommand = QString("RESTORE FROM '%1'").arg(backupPath);

	if (!executeQuery(restoreCommand))
	{
		LOG_ERROR("SQLite恢复命令执行失败", QString("命令: %1").arg(restoreCommand));

		// 恢复失败，回退到原始数据库
		closeDatabase();
		if (hasCurrentBackup)
		{
			QFile::remove(currentDbPath);
			QFile::copy(backupCurrent, currentDbPath);
		}
		openDatabase(currentDbPath);
		return false;
	}

	// 删除临时备份
	if (hasCurrentBackup)
		QFile::remove(backupCurrent);

	LOG_INFO("数据库恢复成功", QString("从备份: %1").arg(backupPath));
	return true;
}

bool DatabaseManager::restoreUsingFileCopy(const QString& backupPath)
{
	if (!QFile::exists(backupPath))
	{
		LOG_ERROR("恢复数据库失败", "备份文件不存在: " + backupPath);
		return false;
	}

	QString currentDbPath = m_database.databaseName();
	if (currentDbPath.isEmpty())
	{
		LOG_ERROR("恢复数据库失败", "当前数据库路径为空");
		return false;
	}

	// 关闭当前数据库连接
	closeDatabase();

	// 备份当前数据库
	QString backupCurrent = currentDbPath + ".backup";
	bool hasCurrentBackup = false;
	if (QFile::exists(currentDbPath))
	{
		if (QFile::copy(currentDbPath, backupCurrent))
			hasCurrentBackup = true;
		else
			LOG_WARN("无法备份当前数据库", "继续恢复操作");
	}

	// 删除当前数据库文件
	if (QFile::exists(currentDbPath) && !QFile::remove(currentDbPath))
	{
		LOG_ERROR("删除当前数据库失败", currentDbPath);
		if (hasCurrentBackup)
			QFile::copy(backupCurrent, currentDbPath);

		openDatabase(currentDbPath);
		return false;
	}

	// 复制备份文件
	if (!QFile::copy(backupPath, currentDbPath))
	{
		LOG_ERROR("复制备份文件失败", QString("从 %1 到 %2").arg(backupPath, currentDbPath));
		if (hasCurrentBackup)
			QFile::copy(backupCurrent, currentDbPath);

		openDatabase(currentDbPath);
		return false;
	}

	// 删除临时备份
	if (hasCurrentBackup)
		QFile::remove(backupCurrent);

	// 重新打开数据库
	if (!openDatabase(currentDbPath))
	{
		LOG_ERROR("重新打开数据库失败", lastError());
		return false;
	}

	LOG_INFO("数据库恢复成功", QString("从备份: %1").arg(backupPath));
	return true;
}

qint64 DatabaseManager::lastInsertId() const
{
	if (!m_database.isOpen())
		return -1;

	QSqlQuery query(m_database);
	if (query.exec("SELECT last_insert_rowid()") && query.next())
		return query.value(0).toLongLong();

	return -1;
}

int DatabaseManager::affectedRows() const
{
	if (!m_database.isOpen())
		return -1;

	QSqlQuery query(m_database);
	if (query.exec("SELECT changes()") && query.next())
		return query.value(0).toInt();

	return -1;
}

QString DatabaseManager::lastError() const
{
	return m_database.lastError().text();
}

QString DatabaseManager::databasePath() const
{
	return m_database.databaseName();
}

qint64 DatabaseManager::databaseSize() const
{
	QFileInfo fileInfo(m_database.databaseName());
	return fileInfo.exists() ? fileInfo.size() : -1;
}

void DatabaseManager::setQueryTimeout(int milliseconds)
{
	m_queryTimeout = milliseconds;
}

int DatabaseManager::queryTimeout() const
{
	return m_queryTimeout;
}

void DatabaseManager::setRetryCount(int count)
{
	m_retryCount = count;
}

int DatabaseManager::retryCount() const
{
	return m_retryCount;
}