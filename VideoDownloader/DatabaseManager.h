#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>
#include <QRecursiveMutex>
#include <QMutexLocker>
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

	// 提供全局互斥锁的访问（用于复杂事务操作）
	QRecursiveMutex& globalMutex() { return m_globalMutex; }

private:
	// 内部实现
	bool initializeDatabase();
	bool executeQueryWithRetry(const QString& query, const QVariantList& params = QVariantList());
	bool executeQueryInternal(QSqlQuery& sqlQuery, const QString& query, const QVariantList& params = QVariantList());
	bool executeSelectInternal(const QString& query,
		const QVariantList& params,
		std::function<void(QSqlQuery&)> resultProcessor);

	bool shouldRetry(const QSqlError& error) const;

	// 备份实现 - 使用 SQLite VACUUM INTO 命令
	bool backupUsingVacuumInto(const QString& backupPath);
	bool restoreUsingFileCopy(const QString& backupPath);

private:
	QSqlDatabase m_database;

	// 全局互斥锁 - 保护所有数据库操作
	mutable QRecursiveMutex m_globalMutex;

	// 配置
	int m_queryTimeout;
	int m_retryCount;

	// 常量定义
	static const QString DEFAULT_DATABASE_NAME;
	static const QString CONNECTION_NAME;
	static const int DEFAULT_QUERY_TIMEOUT;
	static const int DEFAULT_RETRY_COUNT;
};