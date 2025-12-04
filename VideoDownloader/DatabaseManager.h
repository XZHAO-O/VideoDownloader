#pragma once

#include <QSqlDatabase>
#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QMutex>
#include <QReadWriteLock>
#include <QQueue>
#include <QHash>
#include <functional>
#include <memory>
#include <atomic>

class DatabaseManager : public QObject, public std::enable_shared_from_this<DatabaseManager>
{
	Q_OBJECT

public:
	explicit DatabaseManager(QObject* parent = nullptr);
	~DatabaseManager();

	// 禁用拷贝和赋值
	DatabaseManager(const DatabaseManager&) = delete;
	DatabaseManager& operator=(const DatabaseManager&) = delete;

	// 初始化数据库
	bool initialize(const QString& databasePath);

	// SQL执行（通用方法）
	bool executeQuery(const QString& query, const QVariantList& params = QVariantList());
	bool executeSelect(const QString& query,
		const QVariantList& params = QVariantList(),
		std::function<void(QSqlQuery&)> resultProcessor = nullptr);

	// 实用方法
	QString lastError() const;
	QString databasePath() const;
	qint64 databaseSize() const;

	// 事务支持
	bool beginTransaction();
	bool commitTransaction();
	bool rollbackTransaction();

	// 备份和恢复
	bool backupDatabase(const QString& backupPath);
	bool restoreDatabase(const QString& backupPath);

	// 数据库连接
	QSqlDatabase& database() { return m_database; }

	// 检查数据库是否已初始化
	bool isInitialized() const { return m_isInitialized; }

	// 创建表的通用方法
	bool createTable(const QString& tableName, const QString& tableDefinition);

private:
	bool initConnection(const QString& databasePath);

private:
	QSqlDatabase m_database;
	QString m_databasePath;
	mutable QMutex m_mutex;
	bool m_isInitialized;
};