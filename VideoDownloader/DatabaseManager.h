#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>
#include <QHash>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>
#include <memory>

class DatabaseManager : public QObject
{
	Q_OBJECT

public:
	explicit DatabaseManager(QObject* parent = nullptr);
	~DatabaseManager();

	// 禁用拷贝和赋值
	DatabaseManager(const DatabaseManager&) = delete;
	DatabaseManager& operator=(const DatabaseManager&) = delete;

	static DatabaseManager* instance();

	// 数据库连接管理
	bool openDatabase(const QString& databaseName = "VideoDownloader.db");
	void closeDatabase();
	bool isOpen() const;
	bool checkConnectionHealth();
	bool reconnect();

	// 基本表操作
	bool createTable(const QString& tableName, const QString& tableDefinition);
	bool dropTable(const QString& tableName);
	bool tableExists(const QString& tableName);
	bool truncateTable(const QString& tableName);

	// 事务操作
	bool beginTransaction();
	bool commitTransaction();
	bool rollbackTransaction();

	// SQL执行
	bool executeQuery(const QString& query, const QVariantList& params = QVariantList());
	bool executeSelect(const QString& query,
		const QVariantList& params = QVariantList(),
		std::function<void(QSqlQuery&)> resultProcessor = nullptr);

	// 批量操作
	bool executeBatchQuery(const QString& query, const QList<QVariantList>& batchParams);

	// 预编译查询
	bool prepareQuery(const QString& queryName, const QString& query);
	bool executePreparedQuery(const QString& queryName, const QVariantList& params = QVariantList());
	bool executePreparedSelect(const QString& queryName,
		const QVariantList& params = QVariantList(),
		std::function<void(QSqlQuery&)> resultProcessor = nullptr);
	void clearPreparedQueries();
	void cleanupUnusedPreparedQueries();

	// 预编译查询配置
	void setMaxPreparedQueries(int maxQueries);
	int maxPreparedQueries() const;
	int preparedQueryCount() const;

	// 备份和恢复
	bool backupDatabase(const QString& backupPath);
	bool restoreDatabase(const QString& backupPath);

	// 实用方法
	qint64 lastInsertId() const;
	int affectedRows() const;
	QString lastError() const;
	QString databasePath() const;
	qint64 databaseSize() const;

	// 配置
	void setQueryTimeout(int milliseconds);
	int queryTimeout() const;
	void setRetryCount(int count);
	int retryCount() const;

	// RAII事务支持
	class ScopedTransaction
	{
	public:
		explicit ScopedTransaction(DatabaseManager* dbManager);
		~ScopedTransaction();

		bool commit();
		bool isActive() const;
		ScopedTransaction(const ScopedTransaction&) = delete;
		ScopedTransaction& operator=(const ScopedTransaction&) = delete;

	private:
		DatabaseManager* m_dbManager;
		bool m_started;
		bool m_committed;
	};

private slots:
	void cleanupUnusedPreparedQueriesSlot();

private:
	// 内部实现
	bool initializeDatabase();
	bool executeQueryWithRetry(const QString& query, const QVariantList& params = QVariantList());
	bool executeQueryInternal(QSqlQuery& sqlQuery, const QString& query, const QVariantList& params = QVariantList());
	bool executeSelectInternal(const QString& query,
		const QVariantList& params,
		std::function<void(QSqlQuery&)> resultProcessor);

	bool shouldRetry(const QSqlError& error) const;
	int calculateBackoff(int attempt) const;

	// 预编译查询实现
	struct PreparedQuery
	{
		QSqlQuery query;
		qint64 lastUsed;
	};

	// 备份实现 - 使用SQLite命令
	bool backupUsingSqliteBackupCommand(const QString& backupPath);
	bool backupUsingFileCopy(const QString& backupPath);

	// 恢复实现
	bool restoreUsingSqliteRestoreCommand(const QString& backupPath);
	bool restoreUsingFileCopy(const QString& backupPath);

private:
	QSqlDatabase m_database;

	// 预编译查询存储
	QHash<QString, std::shared_ptr<PreparedQuery>> m_preparedQueries;
	QTimer* m_preparedQueryCleanupTimer;
	mutable QMutex m_preparedQueriesMutex;

	// 配置
	int m_queryTimeout;
	int m_retryCount;
	int m_maxPreparedQueries;

	static QMutex m_mutex;
	static DatabaseManager* m_instance;

	// 常量定义
	static const QString DEFAULT_DATABASE_NAME;
	static const QString CONNECTION_NAME;
	static const int DEFAULT_QUERY_TIMEOUT;
	static const int DEFAULT_RETRY_COUNT;
	static const int PREPARED_QUERY_CLEANUP_INTERVAL;
	static const int DEFAULT_MAX_PREPARED_QUERIES;
};