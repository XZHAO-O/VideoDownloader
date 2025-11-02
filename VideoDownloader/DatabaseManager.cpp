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

// ScopedTransaction 实现
DatabaseManager::ScopedTransaction::ScopedTransaction(DatabaseManager* dbManager)
	: m_dbManager(dbManager)
	, m_started(false)
	, m_committed(false)
{
	if (m_dbManager)
	{
		m_started = m_dbManager->beginTransaction();
	}
}

DatabaseManager::ScopedTransaction::~ScopedTransaction()
{
	if (m_started && !m_committed)
	{
		m_dbManager->rollbackTransaction();
	}
}

bool DatabaseManager::ScopedTransaction::commit()
{
	if (m_started && !m_committed)
	{
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
{
	m_database = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
}

DatabaseManager::~DatabaseManager()
{
	QMutexLocker locker(&m_globalMutex);
	// 关闭数据库
	closeDatabase();

	// 安全地移除数据库连接
	if (QSqlDatabase::contains(CONNECTION_NAME))
	{
		QSqlDatabase::removeDatabase(CONNECTION_NAME);
	}
}

bool DatabaseManager::openDatabase(const QString& databaseName)
{
	QMutexLocker locker(&m_globalMutex);

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
	QMutexLocker locker(&m_globalMutex);
	if (m_database.isOpen())
		m_database.close();
}

bool DatabaseManager::isOpen() const
{
	QMutexLocker locker(&m_globalMutex);
	return m_database.isOpen();
}

bool DatabaseManager::checkConnectionHealth()
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
		return false;

	QSqlQuery query(m_database);
	return query.exec("SELECT 1");
}

bool DatabaseManager::reconnect()
{
	QMutexLocker locker(&m_globalMutex);

	QString dbPath = m_database.databaseName();
	closeDatabase();
	return openDatabase(dbPath);
}

bool DatabaseManager::createTable(const QString& tableName, const QString& tableDefinition)
{
	QMutexLocker locker(&m_globalMutex);

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
	QMutexLocker locker(&m_globalMutex);
	return executeQueryWithRetry(QString("DROP TABLE IF EXISTS %1").arg(tableName));
}

bool DatabaseManager::tableExists(const QString& tableName)
{
	QMutexLocker locker(&m_globalMutex);

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
	QMutexLocker locker(&m_globalMutex);

	if (!executeQueryWithRetry(QString("DELETE FROM %1").arg(tableName)))
		return false;

	return executeQueryWithRetry(QString("VACUUM"));
}

bool DatabaseManager::beginTransaction()
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
	{
		LOG_ERROR("开始事务失败", "数据库未打开");
		return false;
	}

	if (!m_database.transaction())
	{
		LOG_ERROR("开始事务失败", lastError());
		return false;
	}

	LOG_DEBUG("事务已经开始", "使用QSqlDatabase事务");
	return true;
}

bool DatabaseManager::commitTransaction()
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
	{
		LOG_ERROR("提交事务失败", "数据库未打开");
		return false;
	}

	if (!m_database.commit())
	{
		LOG_ERROR("提交事务失败", lastError());
		return false;
	}

	return true;
}

bool DatabaseManager::rollbackTransaction()
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
	{
		LOG_ERROR("回滚事务失败", "数据库未打开");
		return false;
	}

	if (!m_database.rollback())
	{
		LOG_ERROR("回滚事务失败", lastError());
		return false;
	}

	return true;
}

bool DatabaseManager::executeQuery(const QString& query, const QVariantList& params)
{
	QMutexLocker locker(&m_globalMutex);
	return executeQueryWithRetry(query, params);
}

bool DatabaseManager::shouldRetry(const QSqlError& error) const
{
	QString errorText = error.text();
	return errorText.contains("locked", Qt::CaseInsensitive) ||
		errorText.contains("busy", Qt::CaseInsensitive) ||
		errorText.contains("timeout", Qt::CaseInsensitive);
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
				LOG_WARN("数据库繁忙，重试中", QString("尝试 %1/%2, 查询: %3")
					.arg(attempts).arg(m_retryCount).arg(query));
				QThread::msleep(100); // 简单等待100ms
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
	QMutexLocker locker(&m_globalMutex);
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

	resultProcessor(sqlQuery);

	return true;
}

bool DatabaseManager::executeBatchQuery(const QString& query, const QList<QVariantList>& batchParams)
{
	QMutexLocker locker(&m_globalMutex);

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

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
	QMutexLocker locker(&m_globalMutex);
	return backupUsingVacuumInto(backupPath);
}

bool DatabaseManager::backupUsingVacuumInto(const QString& backupPath)
{
	if (!m_database.isOpen())
	{
		LOG_ERROR("备份失败", "数据库未打开");
		return false;
	}

	// 参数验证
	if (backupPath.isEmpty())
	{
		LOG_ERROR("备份失败", "备份路径为空");
		return false;
	}

	// 确保备份目录存在
	QFileInfo backupInfo(backupPath);
	QDir backupDir = backupInfo.absoluteDir();
	if (!backupDir.exists() && !backupDir.mkpath("."))
	{
		LOG_ERROR("创建备份目录失败", backupDir.absolutePath());
		return false;
	}

	// 删除已存在的备份文件
	if (QFile::exists(backupPath))
	{
		if (!QFile::remove(backupPath))
		{
			LOG_ERROR("删除已存在的备份文件失败", backupPath);
			return false;
		}
	}

	// 使用 SQLite 的 VACUUM INTO 命令进行备份
	// 这是 SQLite 3.27.0+ 引入的标准命令
	QString vacuumCommand = QString("VACUUM INTO '%1'").arg(backupPath);

	if (!executeQuery(vacuumCommand))
	{
		LOG_ERROR("VACUUM INTO 备份失败", QString("命令: %1, 错误: %2").arg(vacuumCommand, lastError()));
		return false;
	}

	// 验证备份文件
	if (!QFile::exists(backupPath))
	{
		LOG_ERROR("备份文件未创建", backupPath);
		return false;
	}

	qint64 backupSize = QFileInfo(backupPath).size();
	LOG_INFO("数据库备份成功", QString("路径: %1, 大小: %2 字节").arg(backupPath).arg(backupSize));
	return true;
}

bool DatabaseManager::restoreDatabase(const QString& backupPath)
{
	QMutexLocker locker(&m_globalMutex);
	return restoreUsingFileCopy(backupPath);
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

	// 创建带时间戳的临时备份文件名
	QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
	QString tempBackupPath = currentDbPath + ".backup." + timestamp;

	bool success = false;
	bool hasCurrentBackup = false;

	do {
		// 关闭当前数据库连接
		closeDatabase();

		// 备份当前数据库到临时文件
		if (QFile::exists(currentDbPath))
		{
			if (QFile::copy(currentDbPath, tempBackupPath))
			{
				hasCurrentBackup = true;
				LOG_DEBUG("创建当前数据库备份", tempBackupPath);
			}
			else
			{
				LOG_WARN("无法备份当前数据库", "继续恢复操作");
			}
		}

		// 删除当前数据库文件
		if (QFile::exists(currentDbPath) && !QFile::remove(currentDbPath))
		{
			LOG_ERROR("删除当前数据库失败", currentDbPath);
			break;
		}

		// 复制备份文件到当前数据库位置
		if (!QFile::copy(backupPath, currentDbPath))
		{
			LOG_ERROR("复制备份文件失败",
				QString("从 %1 到 %2").arg(backupPath, currentDbPath));
			break;
		}

		// 设置正确的文件权限
		QFile::setPermissions(currentDbPath,
			QFile::ReadOwner | QFile::WriteOwner |
			QFile::ReadUser | QFile::WriteUser);

		// 重新打开数据库
		if (!openDatabase(currentDbPath))
		{
			LOG_ERROR("重新打开数据库失败", currentDbPath);
			break;
		}

		// 验证恢复的数据库
		if (!checkConnectionHealth())
		{
			LOG_ERROR("恢复的数据库连接验证失败", "");
			closeDatabase();
			break;
		}

		success = true;

	} while (false);

	// 恢复失败时的回退操作
	if (!success)
	{
		// 关闭可能已打开的数据库
		closeDatabase();

		// 删除可能已创建的不完整数据库文件
		if (QFile::exists(currentDbPath))
		{
			QFile::remove(currentDbPath);
		}

		// 从临时备份恢复原始数据库
		if (hasCurrentBackup && QFile::exists(tempBackupPath))
		{
			if (QFile::copy(tempBackupPath, currentDbPath))
			{
				LOG_INFO("已从临时备份恢复原始数据库", tempBackupPath);
				openDatabase(currentDbPath);
			}
		}
	}

	// 清理临时备份文件
	if (hasCurrentBackup && QFile::exists(tempBackupPath))
	{
		QFile::remove(tempBackupPath);
		LOG_DEBUG("删除临时备份文件", tempBackupPath);
	}

	if (success)
	{
		LOG_INFO("数据库恢复成功", QString("从备份: %1").arg(backupPath));
	}
	else
	{
		LOG_ERROR("数据库恢复失败", QString("备份文件: %1").arg(backupPath));
	}

	return success;
}

qint64 DatabaseManager::lastInsertId() const
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
		return -1;

	QSqlQuery query(m_database);
	if (query.exec("SELECT last_insert_rowid()") && query.next())
		return query.value(0).toLongLong();

	return -1;
}

int DatabaseManager::affectedRows() const
{
	QMutexLocker locker(&m_globalMutex);

	if (!m_database.isOpen())
		return -1;

	QSqlQuery query(m_database);
	if (query.exec("SELECT changes()") && query.next())
		return query.value(0).toInt();

	return -1;
}

QString DatabaseManager::lastError() const
{
	QMutexLocker locker(&m_globalMutex);
	return m_database.lastError().text();
}

QString DatabaseManager::databasePath() const
{
	QMutexLocker locker(&m_globalMutex);
	return m_database.databaseName();
}

qint64 DatabaseManager::databaseSize() const
{
	QMutexLocker locker(&m_globalMutex);
	QFileInfo fileInfo(m_database.databaseName());
	return fileInfo.exists() ? fileInfo.size() : -1;
}

void DatabaseManager::setQueryTimeout(int milliseconds)
{
	QMutexLocker locker(&m_globalMutex);
	m_queryTimeout = milliseconds;
}

int DatabaseManager::queryTimeout() const
{
	QMutexLocker locker(&m_globalMutex);
	return m_queryTimeout;
}

void DatabaseManager::setRetryCount(int count)
{
	QMutexLocker locker(&m_globalMutex);
	m_retryCount = count;
}

int DatabaseManager::retryCount() const
{
	QMutexLocker locker(&m_globalMutex);
	return m_retryCount;
}