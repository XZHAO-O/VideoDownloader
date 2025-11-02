#pragma once

#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include <QHash>
#include <QTimer>
#include <QElapsedTimer>
#include <QThreadStorage>
#include <functional>
#include <memory>
#include <atomic>

class DatabaseConnection
{
public:
	explicit DatabaseConnection(const QString& connectionName);
	~DatabaseConnection();

	bool open(const QString& databasePath);
	void close();
	bool isOpen() const;
	bool isValid() const;
	bool initialize();

	QSqlDatabase& database() { return m_database; }
	const QSqlDatabase& database() const { return m_database; }
	QString connectionName() const { return m_connectionName; }
	QString lastError() const { return m_database.lastError().text(); }

	// 连接状态管理
	void setInUse(bool inUse) { m_inUse = inUse; }
	bool isInUse() const { return m_inUse; }
	void updateLastUsed() { m_lastUsed = QDateTime::currentDateTime(); }
	QDateTime lastUsed() const { return m_lastUsed; }

private:
	QSqlDatabase m_database;
	QString m_connectionName;
	std::atomic<bool> m_inUse{ false };
	QDateTime m_lastUsed;
};

class DatabaseManager : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseManager(QObject* parent = nullptr);
	~DatabaseManager();

	// 禁用拷贝和赋值
	DatabaseManager(const DatabaseManager&) = delete;
	DatabaseManager& operator=(const DatabaseManager&) = delete;

	// 连接池管理
	bool initializeConnectionPool(const QString& databaseName = "VideoDownloader.db",
		int poolSize = 5);
	void closeAllConnections();

	// 连接获取和释放
	std::shared_ptr<DatabaseConnection> acquireConnection();
	void releaseConnection(const std::shared_ptr<DatabaseConnection>& connection);

	// 基本表操作
	bool createTable(const QString& tableName, const QString& tableDefinition);
	bool dropTable(const QString& tableName);
	bool tableExists(const QString& tableName);
	bool truncateTable(const QString& tableName);

	// 事务操作 - 使用单个连接
	bool beginTransaction(const std::shared_ptr<DatabaseConnection>& connection);
	bool commitTransaction(const std::shared_ptr<DatabaseConnection>& connection);
	bool rollbackTransaction(const std::shared_ptr<DatabaseConnection>& connection);

	// SQL执行 - 自动获取连接
	bool executeQuery(const QString& query, const QVariantList& params = QVariantList());
	bool executeSelect(const QString& query,
		const QVariantList& params = QVariantList(),
		std::function<void(QSqlQuery&)> resultProcessor = nullptr);

	// 使用指定连接执行SQL
	bool executeQueryWithConnection(const std::shared_ptr<DatabaseConnection>& connection,
		const QString& query,
		const QVariantList& params = QVariantList());
	bool executeSelectWithConnection(const std::shared_ptr<DatabaseConnection>& connection,
		const QString& query,
		const QVariantList& params = QVariantList(),
		std::function<void(QSqlQuery&)> resultProcessor = nullptr);

	// 批量操作
	bool executeBatchQuery(const QString& query, const QList<QVariantList>& batchParams);

	// 备份和恢复
	bool backupDatabase(const QString& backupPath);
	bool restoreDatabase(const QString& backupPath);

	// 连接池状态
	int activeConnectionCount() const;
	int idleConnectionCount() const;
	int totalConnectionCount() const;
	bool isConnectionPoolHealthy() const;

	// 连接池维护
	bool cleanupIdleConnections();

	// 实用方法
	QString lastError() const;
	QString databasePath() const;
	qint64 databaseSize() const;

	// 配置
	void setQueryTimeout(int milliseconds);
	int queryTimeout() const;
	void setRetryCount(int count);
	int retryCount() const;
	void setConnectionPoolSize(int size);
	int connectionPoolSize() const;

	// RAII事务支持
	class ScopedTransaction
	{
	public:
		explicit ScopedTransaction(DatabaseManager* dbManager);
		~ScopedTransaction();

		bool commit();
		bool isActive() const;
		std::shared_ptr<DatabaseConnection> connection() const { return m_connection; }

		ScopedTransaction(const ScopedTransaction&) = delete;
		ScopedTransaction& operator=(const ScopedTransaction&) = delete;

	private:
		DatabaseManager* m_dbManager;
		std::shared_ptr<DatabaseConnection> m_connection;
		bool m_started;
		bool m_committed;
	};

	// RAII连接支持
	class ScopedConnection
	{
	public:
		explicit ScopedConnection(DatabaseManager* dbManager);
		~ScopedConnection();

		std::shared_ptr<DatabaseConnection> connection() const { return m_connection; }
		bool isValid() const { return m_connection != nullptr; }

		ScopedConnection(const ScopedConnection&) = delete;
		ScopedConnection& operator=(const ScopedConnection&) = delete;

	private:
		DatabaseManager* m_dbManager;
		std::shared_ptr<DatabaseConnection> m_connection;
	};

private:
	// 内部实现
	bool initializeDatabaseConnection(DatabaseConnection* connection);
	std::shared_ptr<DatabaseConnection> createConnection();

	bool executeQueryWithRetry(const std::shared_ptr<DatabaseConnection>& connection,
		const QString& query,
		const QVariantList& params = QVariantList());
	bool executeQueryInternal(QSqlQuery& sqlQuery,
		const QString& query,
		const QVariantList& params = QVariantList());

	bool shouldRetry(const QSqlError& error) const;

	// 备份实现
	bool backupUsingVacuumInto(const QString& backupPath);
	bool restoreUsingFileCopy(const QString& backupPath);

private:
	// 连接池
	QQueue<std::shared_ptr<DatabaseConnection>> m_idleConnections;
	QHash<QString, std::shared_ptr<DatabaseConnection>> m_allConnections;
	mutable QReadWriteLock m_connectionPoolLock;

	// 配置
	QString m_databasePath;
	int m_queryTimeout;
	int m_retryCount;
	int m_connectionPoolSize;
	int m_maxConnectionPoolSize;

	// 常量定义
	static const QString DEFAULT_DATABASE_NAME;
	static const int DEFAULT_QUERY_TIMEOUT;
	static const int DEFAULT_RETRY_COUNT;
	static const int DEFAULT_POOL_SIZE;
	static const int MAX_POOL_SIZE;
};