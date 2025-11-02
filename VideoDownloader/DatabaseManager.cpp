#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <QReadWriteLock>
#include <memory>
#include <algorithm>

#include "LogSystem.h"

// 常量定义
const QString DatabaseManager::DEFAULT_DATABASE_NAME = "VideoDownloader.db";
const int DatabaseManager::DEFAULT_QUERY_TIMEOUT = 30000; // 30秒
const int DatabaseManager::DEFAULT_RETRY_COUNT = 3;
const int DatabaseManager::DEFAULT_POOL_SIZE = 5;
const int DatabaseManager::MAX_POOL_SIZE = 20;

// DatabaseConnection 实现
DatabaseConnection::DatabaseConnection(const QString& connectionName)
	: m_connectionName(connectionName)
{
	m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
}

DatabaseConnection::~DatabaseConnection()
{
	close();

	if (QSqlDatabase::contains(m_connectionName))
	{
		QSqlDatabase::removeDatabase(m_connectionName);
	}
}

bool DatabaseConnection::open(const QString& databasePath)
{
	if (m_database.isOpen())
		return true;

	m_database.setDatabaseName(databasePath);
	if (!m_database.open())
	{
		LOG_ERROR("数据库连接打开失败",
			QString("连接: %1, 路径: %2, 错误: %3")
			.arg(m_connectionName, databasePath, m_database.lastError().text()));
		return false;
	}

	return initialize();
}

void DatabaseConnection::close()
{
	if (m_database.isOpen())
		m_database.close();
}

bool DatabaseConnection::isOpen() const
{
	return m_database.isOpen();
}

bool DatabaseConnection::isValid() const
{
	if (!isOpen())
		return false;

	// 执行简单查询测试连接有效性
	QSqlQuery query(m_database);
	return query.exec("SELECT 1");
}

bool DatabaseConnection::initialize()
{
	// 启用外键约束
	QSqlQuery query(m_database);
	if (!query.exec("PRAGMA foreign_keys = ON"))
	{
		LOG_ERROR("外键约束启用失败",
			QString("连接: %1, 错误: %2").arg(m_connectionName, query.lastError().text()));
		return false;
	}

	// 设置WAL模式提高并发性能
	if (!query.exec("PRAGMA journal_mode = WAL"))
	{
		LOG_WARN("WAL模式设置失败",
			QString("连接: %1, 错误: %2").arg(m_connectionName, query.lastError().text()));
	}

	if (!query.exec("PRAGMA synchronous = NORMAL"))
	{
		LOG_WARN("同步模式设置失败",
			QString("连接: %1, 错误: %2").arg(m_connectionName, query.lastError().text()));
	}

	if (!query.exec("PRAGMA cache_size = -64000"))
	{
		LOG_WARN("缓存大小设置失败",
			QString("连接: %1, 错误: %2").arg(m_connectionName, query.lastError().text()));
	}

	if (!query.exec("PRAGMA busy_timeout = 5000"))
	{
		LOG_WARN("设置繁忙超时失败",
			QString("连接: %1, 错误: %2").arg(m_connectionName, query.lastError().text()));
	}

	return true;
}

// ScopedTransaction 实现
DatabaseManager::ScopedTransaction::ScopedTransaction(DatabaseManager* dbManager)
	: m_dbManager(dbManager)
	, m_started(false)
	, m_committed(false)
{
	if (m_dbManager)
	{
		m_connection = m_dbManager->acquireConnection();
		if (m_connection)
		{
			m_started = m_dbManager->beginTransaction(m_connection);
		}
	}
}

DatabaseManager::ScopedTransaction::~ScopedTransaction()
{
	if (m_started && !m_committed && m_connection)
	{
		m_dbManager->rollbackTransaction(m_connection);
	}

	if (m_connection)
	{
		m_dbManager->releaseConnection(m_connection);
	}
}

bool DatabaseManager::ScopedTransaction::commit()
{
	if (m_started && !m_committed && m_connection)
	{
		m_committed = m_dbManager->commitTransaction(m_connection);
		return m_committed;
	}
	return false;
}

bool DatabaseManager::ScopedTransaction::isActive() const
{
	return m_started && !m_committed;
}

// ScopedConnection 实现
DatabaseManager::ScopedConnection::ScopedConnection(DatabaseManager* dbManager)
	: m_dbManager(dbManager)
{
	if (m_dbManager)
	{
		m_connection = m_dbManager->acquireConnection();
	}
}

DatabaseManager::ScopedConnection::~ScopedConnection()
{
	if (m_connection && m_dbManager)
	{
		m_dbManager->releaseConnection(m_connection);
	}
}

// DatabaseManager 实现
DatabaseManager::DatabaseManager(QObject* parent)
	: QObject(parent)
	, m_queryTimeout(DEFAULT_QUERY_TIMEOUT)
	, m_retryCount(DEFAULT_RETRY_COUNT)
	, m_connectionPoolSize(DEFAULT_POOL_SIZE)
	, m_maxConnectionPoolSize(MAX_POOL_SIZE)
{
}

DatabaseManager::~DatabaseManager()
{
	closeAllConnections();
}

bool DatabaseManager::initializeConnectionPool(const QString& databaseName, int poolSize)
{
	QWriteLocker locker(&m_connectionPoolLock);

	// 关闭现有连接
	closeAllConnections();

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

	m_databasePath = dir.filePath(databaseName.isEmpty() ? DEFAULT_DATABASE_NAME : databaseName);
	m_connectionPoolSize = qMin(poolSize, m_maxConnectionPoolSize);

	// 创建连接池
	for (int i = 0; i < m_connectionPoolSize; ++i)
	{
		auto connection = createConnection();
		if (!connection)
		{
			LOG_ERROR("创建数据库连接失败", QString("索引: %1").arg(i));
			closeAllConnections();
			return false;
		}

		m_idleConnections.enqueue(connection);
		m_allConnections.insert(connection->connectionName(), connection);
	}

	LOG_INFO("数据库连接池初始化成功",
		QString("连接数: %1, 数据库路径: %2").arg(m_connectionPoolSize).arg(m_databasePath));
	return true;
}

void DatabaseManager::closeAllConnections()
{
	QWriteLocker locker(&m_connectionPoolLock);

	m_idleConnections.clear();
	m_allConnections.clear();
}

std::shared_ptr<DatabaseConnection> DatabaseManager::acquireConnection()
{
	QWriteLocker locker(&m_connectionPoolLock);

	// 首先尝试从空闲队列获取有效连接
	while (!m_idleConnections.isEmpty())
	{
		auto connection = m_idleConnections.dequeue();

		// 检查连接是否有效
		if (connection->isValid() && !connection->isInUse())
		{
			connection->setInUse(true);
			connection->updateLastUsed();
			return connection;
		}
		else
		{
			// 连接无效，从总连接映射中移除
			LOG_WARN("移除无效数据库连接", connection->connectionName());
			m_allConnections.remove(connection->connectionName());
			// connection 智能指针离开作用域会自动销毁
		}
	}

	// 如果没有空闲连接且未达到最大限制，创建新连接
	if (m_allConnections.size() < m_maxConnectionPoolSize)
	{
		auto connection = createConnection();
		if (connection && connection->isValid())
		{
			connection->setInUse(true);
			connection->updateLastUsed();
			m_allConnections.insert(connection->connectionName(), connection);
			return connection;
		}
		else
		{
			LOG_ERROR("创建新数据库连接失败", "连接无效");
			return nullptr;
		}
	}

	LOG_ERROR("获取数据库连接失败", "连接池已耗尽");
	return nullptr;
}

void DatabaseManager::releaseConnection(const std::shared_ptr<DatabaseConnection>& connection)
{
	if (!connection)
		return;

	QWriteLocker locker(&m_connectionPoolLock);

	connection->setInUse(false);
	connection->updateLastUsed();

	// 检查连接是否仍然有效
	if (!connection->isValid())
	{
		LOG_WARN("释放无效数据库连接", connection->connectionName());
		m_allConnections.remove(connection->connectionName());
		return;
	}

	// 如果连接池未满且连接有效，放回空闲队列
	if (m_idleConnections.size() < m_connectionPoolSize)
	{
		m_idleConnections.enqueue(connection);
	}
	else
	{
		// 连接池已满，移除最旧的空闲连接（如果存在）并加入当前连接
		if (!m_idleConnections.isEmpty())
		{
			auto oldestConnection = m_idleConnections.dequeue();
			m_allConnections.remove(oldestConnection->connectionName());
			LOG_DEBUG("移除最旧空闲连接", oldestConnection->connectionName());
		}

		m_idleConnections.enqueue(connection);
	}
}

bool DatabaseManager::cleanupIdleConnections()
{
	QWriteLocker locker(&m_connectionPoolLock);

	int initialSize = m_idleConnections.size();
	QDateTime now = QDateTime::currentDateTime();

	// 清理超时空闲连接（30分钟）
	const int MAX_IDLE_MINUTES = 30;

	QQueue<std::shared_ptr<DatabaseConnection>> validConnections;
	int removedCount = 0;

	while (!m_idleConnections.isEmpty())
	{
		auto connection = m_idleConnections.dequeue();

		// 检查连接是否超时或无效
		if (connection->lastUsed().secsTo(now) > MAX_IDLE_MINUTES * 60 ||
			!connection->isValid())
		{
			m_allConnections.remove(connection->connectionName());
			removedCount++;
			LOG_DEBUG("清理空闲连接",
				QString("连接: %1, 空闲时间: %2分钟")
				.arg(connection->connectionName())
				.arg(connection->lastUsed().secsTo(now) / 60));
		}
		else
		{
			validConnections.enqueue(connection);
		}
	}

	m_idleConnections = validConnections;

	if (removedCount > 0)
	{
		LOG_INFO("清理空闲连接完成",
			QString("清理数量: %1, 剩余空闲连接: %2")
			.arg(removedCount)
			.arg(m_idleConnections.size()));
	}

	return removedCount > 0;
}

std::shared_ptr<DatabaseConnection> DatabaseManager::createConnection()
{
	static std::atomic<int> connectionCounter(0);
	QString connectionName = QString("VideoDownloader_connection_%1_%2")
		.arg(connectionCounter.fetch_add(1))
		.arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

	auto connection = std::make_shared<DatabaseConnection>(connectionName);
	if (!connection->open(m_databasePath))
	{
		LOG_ERROR("创建数据库连接失败", QString("连接名: %1").arg(connectionName));
		return nullptr;
	}

	return connection;
}

bool DatabaseManager::isConnectionPoolHealthy() const
{
	QReadLocker locker(&m_connectionPoolLock);

	int validConnections = 0;
	for (const auto& connection : m_allConnections)
	{
		if (connection->isValid())
			validConnections++;
	}

	bool healthy = (validConnections >= m_connectionPoolSize * 0.8); // 80%连接有效视为健康
	if (!healthy)
	{
		LOG_WARN("连接池健康状态不佳",
			QString("有效连接: %1/%2, 要求最小: %3")
			.arg(validConnections)
			.arg(m_allConnections.size())
			.arg(m_connectionPoolSize * 0.8));
	}

	return healthy;
}

bool DatabaseManager::createTable(const QString& tableName, const QString& tableDefinition)
{
	if (tableName.isEmpty() || tableDefinition.isEmpty())
	{
		LOG_ERROR("创建表失败", "表名或表定义为空");
		return false;
	}

	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	QString query = QString("CREATE TABLE IF NOT EXISTS %1 (%2)").arg(tableName, tableDefinition);
	return executeQueryWithConnection(scopedConn.connection(), query);
}

bool DatabaseManager::dropTable(const QString& tableName)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	return executeQueryWithConnection(scopedConn.connection(),
		QString("DROP TABLE IF EXISTS %1").arg(tableName));
}

bool DatabaseManager::tableExists(const QString& tableName)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	bool exists = false;
	QString query = "SELECT name FROM sqlite_master WHERE type='table' AND name=?";

	return executeSelectWithConnection(scopedConn.connection(), query, { tableName },
		[&](QSqlQuery& result) {
			exists = result.next() && (result.value(0).toString() == tableName);
		});
}

bool DatabaseManager::truncateTable(const QString& tableName)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	return executeQueryWithConnection(scopedConn.connection(),
		QString("DELETE FROM %1").arg(tableName));
}

bool DatabaseManager::beginTransaction(const std::shared_ptr<DatabaseConnection>& connection)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("开始事务失败", "数据库连接无效");
		return false;
	}

	QSqlDatabase& db = connection->database();
	if (!db.transaction())
	{
		LOG_ERROR("开始事务失败", QString("连接: %1, 错误: %2")
			.arg(connection->connectionName(), db.lastError().text()));
		return false;
	}

	return true;
}

bool DatabaseManager::commitTransaction(const std::shared_ptr<DatabaseConnection>& connection)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("提交事务失败", "数据库连接无效");
		return false;
	}

	QSqlDatabase& db = connection->database();
	if (!db.commit())
	{
		LOG_ERROR("提交事务失败", QString("连接: %1, 错误: %2")
			.arg(connection->connectionName(), db.lastError().text()));
		return false;
	}

	return true;
}

bool DatabaseManager::rollbackTransaction(const std::shared_ptr<DatabaseConnection>& connection)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("回滚事务失败", "数据库连接无效");
		return false;
	}

	QSqlDatabase& db = connection->database();
	if (!db.rollback())
	{
		LOG_ERROR("回滚事务失败", QString("连接: %1, 错误: %2")
			.arg(connection->connectionName(), db.lastError().text()));
		return false;
	}

	return true;
}

bool DatabaseManager::executeQuery(const QString& query, const QVariantList& params)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	return executeQueryWithConnection(scopedConn.connection(), query, params);
}

bool DatabaseManager::executeSelect(const QString& query, const QVariantList& params,
	std::function<void(QSqlQuery&)> resultProcessor)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	return executeSelectWithConnection(scopedConn.connection(), query, params, resultProcessor);
}

bool DatabaseManager::executeQueryWithConnection(const std::shared_ptr<DatabaseConnection>& connection,
	const QString& query,
	const QVariantList& params)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("执行SQL失败", "数据库连接无效");
		return false;
	}

	return executeQueryWithRetry(connection, query, params);
}

bool DatabaseManager::executeSelectWithConnection(const std::shared_ptr<DatabaseConnection>& connection,
	const QString& query,
	const QVariantList& params,
	std::function<void(QSqlQuery&)> resultProcessor)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("执行查询失败", "数据库连接无效");
		return false;
	}

	if (!resultProcessor)
		return false;

	QSqlDatabase& db = connection->database();
	QSqlQuery sqlQuery(db);
	sqlQuery.setForwardOnly(true);

	if (!sqlQuery.prepare(query))
	{
		LOG_ERROR("SQL准备失败", QString("连接: %1, Query: %2, Error: %3")
			.arg(connection->connectionName(), query, sqlQuery.lastError().text()));
		return false;
	}

	for (int i = 0; i < params.size(); ++i)
		sqlQuery.bindValue(i, params[i]);

	if (!sqlQuery.exec())
	{
		LOG_ERROR("SQL查询错误", QString("连接: %1, Query: %2, Error: %3")
			.arg(connection->connectionName(), query, sqlQuery.lastError().text()));
		return false;
	}

	resultProcessor(sqlQuery);
	return true;
}

bool DatabaseManager::executeBatchQuery(const QString& query, const QList<QVariantList>& batchParams)
{
	ScopedTransaction transaction(this);
	if (!transaction.isActive())
		return false;

	for (const QVariantList& params : batchParams)
	{
		if (!executeQueryWithConnection(transaction.connection(), query, params))
		{
			return false;
		}
	}

	return transaction.commit();
}

bool DatabaseManager::executeQueryWithRetry(const std::shared_ptr<DatabaseConnection>& connection,
	const QString& query,
	const QVariantList& params)
{
	if (!connection || !connection->isValid())
	{
		LOG_ERROR("执行SQL失败", "数据库连接无效");
		return false;
	}

	int attempts = 0;
	while (attempts <= m_retryCount)
	{
		QSqlDatabase& db = connection->database();
		QSqlQuery sqlQuery(db);

		if (executeQueryInternal(sqlQuery, query, params))
		{
			return true;
		}

		if (shouldRetry(sqlQuery.lastError()))
		{
			attempts++;
			if (attempts <= m_retryCount)
			{
				LOG_WARN("数据库繁忙，重试中",
					QString("连接: %1, 尝试 %2/%3, 查询: %4")
					.arg(connection->connectionName()).arg(attempts).arg(m_retryCount).arg(query));
				QThread::msleep(100);
				continue;
			}
		}

		break;
	}

	QString error = connection->lastError();
	LOG_ERROR("SQL执行失败，已达到最大重试次数",
		QString("连接: %1, 查询: %2, 错误: %3")
		.arg(connection->connectionName(), query, error));
	return false;
}

bool DatabaseManager::executeQueryInternal(QSqlQuery& sqlQuery,
	const QString& query,
	const QVariantList& params)
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

bool DatabaseManager::shouldRetry(const QSqlError& error) const
{
	QString errorText = error.text();
	return errorText.contains("locked", Qt::CaseInsensitive) ||
		errorText.contains("busy", Qt::CaseInsensitive) ||
		errorText.contains("timeout", Qt::CaseInsensitive);
}

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
	ScopedConnection scopedConn(this);
	if (!scopedConn.isValid())
		return false;

	// 使用SQLite的备份API进行备份
	QString backupQuery = QString("VACUUM INTO '%1'").arg(backupPath);
	return executeQueryWithConnection(scopedConn.connection(), backupQuery);
}

bool DatabaseManager::restoreDatabase(const QString& backupPath)
{
	QWriteLocker locker(&m_connectionPoolLock);

	// 关闭所有连接
	closeAllConnections();

	// 检查备份文件是否存在
	if (!QFile::exists(backupPath))
	{
		LOG_ERROR("恢复数据库失败", "备份文件不存在");
		return false;
	}

	// 复制备份文件到数据库路径
	if (QFile::exists(m_databasePath))
	{
		if (!QFile::remove(m_databasePath))
		{
			LOG_ERROR("恢复数据库失败", "无法删除现有数据库文件");
			return false;
		}
	}

	if (!QFile::copy(backupPath, m_databasePath))
	{
		LOG_ERROR("恢复数据库失败", "无法复制备份文件");
		return false;
	}

	// 重新初始化连接池
	return initializeConnectionPool(QFileInfo(m_databasePath).fileName(), m_connectionPoolSize);
}

int DatabaseManager::activeConnectionCount() const
{
	QReadLocker locker(&m_connectionPoolLock);
	int count = 0;
	for (const auto& connection : m_allConnections)
	{
		if (connection->isInUse())
			count++;
	}
	return count;
}

int DatabaseManager::idleConnectionCount() const
{
	QReadLocker locker(&m_connectionPoolLock);
	return m_idleConnections.size();
}

int DatabaseManager::totalConnectionCount() const
{
	QReadLocker locker(&m_connectionPoolLock);
	return m_allConnections.size();
}

QString DatabaseManager::lastError() const
{
	// 在实际应用中可能需要更复杂的错误收集机制
	// 这里返回一个通用的错误信息，具体错误应该在每个操作中记录
	return "检查具体操作的错误日志";
}

QString DatabaseManager::databasePath() const
{
	return m_databasePath;
}

qint64 DatabaseManager::databaseSize() const
{
	QFileInfo fileInfo(m_databasePath);
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

void DatabaseManager::setConnectionPoolSize(int size)
{
	m_connectionPoolSize = qMin(size, m_maxConnectionPoolSize);
}

int DatabaseManager::connectionPoolSize() const
{
	return m_connectionPoolSize;
}

bool DatabaseManager::initializeDatabaseConnection(DatabaseConnection* connection)
{
	if (!connection)
		return false;

	return connection->initialize();
}

bool DatabaseManager::backupUsingVacuumInto(const QString& backupPath)
{
	return backupDatabase(backupPath);
}

bool DatabaseManager::restoreUsingFileCopy(const QString& backupPath)
{
	return restoreDatabase(backupPath);
}