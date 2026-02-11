// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_executor.h"

// Qt Core
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>

// Project internal headers
#include "Logger.h"

namespace {
	constexpr const char* kDatabaseName = "nexusdl.db";
}

namespace nexusdl::database {

	// 静态thread_local成员初始化
	thread_local std::unordered_map<std::string, DatabaseExecutor::ThreadLocalData> DatabaseExecutor::s_threadLocalDatabases;

	DatabaseExecutor::DatabaseExecutor(QObject* parent)
		: QObject{ parent }
		, m_databasePath{ QCoreApplication::applicationDirPath() % "/database" }
	{
		initialize();
	}

	DatabaseExecutor::DatabaseExecutor(const QString& databasePath, QObject* parent)
		: QObject{ parent }
		, m_databasePath{ databasePath }
	{
		initialize();
	}

	DatabaseExecutor::~DatabaseExecutor()
	{
		QWriteLocker locker(&m_dataLock);
		auto& threadData = getThreadLocalData();
		if (threadData.database.isOpen())
		{
			threadData.database.close();
		}
	}

	bool DatabaseExecutor::initialize()
	{
		auto& threadData = getThreadLocalData();

		if (threadData.isInitialized)
		{
			return true;
		}

		// 检查并创建数据库目录
		if (QDir dir{ m_databasePath }; !dir.exists())
		{
			if (!dir.mkpath("."))
			{
				LOG_ERROR(QString{ "Failed to create database directory: " % m_databasePath });
				return false;
			}
		}

		// 初始化数据库连接
		if (!initConnection())
		{
			return false;
		}

		threadData.isInitialized = true;
		LOG_INFO(QString{ "Database initialized successfully at: " % m_databasePath });
		return true;
	}

	bool DatabaseExecutor::initConnection()
	{
		QWriteLocker locker(&m_dataLock);
		auto& threadData = getThreadLocalData();

		const QString fullDatabaseName{ m_databasePath % "/" % kDatabaseName };
		const bool exists = QFile::exists(fullDatabaseName);

		// 使用数据库路径和线程ID作为连接名，确保每个线程每个数据库有独立的连接
		QString connectionName{
			"NexusDLDBConnection_" %
			QString::fromStdString(std::to_string(std::hash<std::string>{}(m_databasePath.toStdString()))) % "_" %
			QString::number(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())))
		};

		// 如果连接已存在，先移除
		if (QSqlDatabase::contains(connectionName))
		{
			QSqlDatabase::removeDatabase(connectionName);
		}

		threadData.database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
		threadData.database.setDatabaseName(fullDatabaseName);

		if (!threadData.database.open())
		{
			LOG_ERROR(QString{ "Failed to open database: " % threadData.database.lastError().text() });
			return false;
		}

		// 如果是新创建的数据库，设置SQLite参数
		if (!exists)
		{
			QSqlQuery query{ threadData.database };
			query.exec("PRAGMA foreign_keys = ON");
			query.exec("PRAGMA journal_mode = WAL");
			query.exec("PRAGMA synchronous = NORMAL");
			query.exec("PRAGMA cache_size = -64000");
			query.exec("PRAGMA busy_timeout = 5000");

			LOG_DEBUG("SQLite database created with optimized settings");
		}
		else
		{
			LOG_DEBUG("Existing database opened");
		}

		return true;
	}

	bool DatabaseExecutor::ensureConnection()
	{
		return initialize();
	}

	DatabaseExecutor::ThreadLocalData& DatabaseExecutor::getThreadLocalData()
	{
		std::string key = m_databasePath.toStdString();
		return s_threadLocalDatabases[key];
	}

	const DatabaseExecutor::ThreadLocalData& DatabaseExecutor::getThreadLocalData() const
	{
		std::string key = m_databasePath.toStdString();
		return s_threadLocalDatabases[key];
	}

	bool DatabaseExecutor::executeQuery(const QString& queryStr, const QVariantMap& params)
	{
		QWriteLocker locker{ &m_dataLock };
		auto& threadData = getThreadLocalData();

		if (!threadData.database.isOpen())
		{
			LOG_ERROR("Database is not open");
			return false;
		}

		QSqlQuery query{ threadData.database };
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return false;
		}

		return true;
	}

	bool DatabaseExecutor::executeQuery(const QString& queryStr, const QVariantList& params)
	{
		QWriteLocker locker{ &m_dataLock };
		auto& threadData = getThreadLocalData();

		if (!threadData.database.isOpen())
		{
			LOG_ERROR("Database is not open");
			return false;
		}

		QSqlQuery query{ threadData.database };
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i)
		{
			query.bindValue(i, params[i]);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return false;
		}

		return true;
	}

	QList<QVariantMap> DatabaseExecutor::executeQueryToMap(const QString& queryStr, const QVariantMap& params)
	{
		QReadLocker locker{ &m_dataLock };
		auto& threadData = getThreadLocalData();

		QList<QVariantMap> result{};

		if (!threadData.database.isOpen())
		{
			LOG_ERROR("Database is not open");
			return result;
		}

		QSqlQuery query{ threadData.database };
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute select query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return result;
		}

		while (query.next())
		{
			QVariantMap row{};
			QSqlRecord record = query.record();

			for (int i = 0; i < record.count(); ++i)
			{
				row[record.fieldName(i)] = query.value(i);
			}

			result.append(row);
		}

		LOG_DEBUG(QString{ "Query returned " % QString::number(result.size()) % " rows" });
		return result;
	}

	QList<QVariantMap> DatabaseExecutor::executeQueryToMap(const QString& queryStr, const QVariantList& params)
	{
		QReadLocker locker{ &m_dataLock };
		auto& threadData = getThreadLocalData();

		QList<QVariantMap> result{};

		if (!threadData.database.isOpen())
		{
			LOG_ERROR("Database is not open");
			return result;
		}

		QSqlQuery query{ threadData.database };
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i)
		{
			query.bindValue(i, params[i]);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute select query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return result;
		}

		while (query.next())
		{
			QVariantMap row{};
			QSqlRecord record = query.record();

			for (int i = 0; i < record.count(); ++i)
			{
				row[record.fieldName(i)] = query.value(i);
			}

			result.append(row);
		}

		LOG_DEBUG(QString{ "Query returned " % QString::number(result.size()) % " rows" });
		return result;
	}

	QString DatabaseExecutor::lastError() const
	{
		QReadLocker locker{ &m_dataLock };
		auto& threadData = getThreadLocalData();
		return threadData.database.lastError().text();
	}

	QString DatabaseExecutor::databasePath() const
	{
		return m_databasePath;
	}

	qint64 DatabaseExecutor::databaseSize() const
	{
		if (QFileInfo fileInfo(m_databasePath % "/" % kDatabaseName); fileInfo.exists())
		{
			return fileInfo.size();
		}
		else
		{
			return -1;
		}
	}

	bool DatabaseExecutor::beginTransaction()
	{
		m_dataLock.lockForWrite();
		auto& threadData = getThreadLocalData();
		bool result = threadData.database.transaction();
		if (!result)
		{
			LOG_ERROR(QString{ "Failed to begin transaction: " % threadData.database.lastError().text() });
			m_dataLock.unlock();
		}
		else
		{
			LOG_DEBUG("Database transaction started");
		}
		return result;
	}

	bool DatabaseExecutor::commitTransaction()
	{
		auto& threadData = getThreadLocalData();
		bool result = threadData.database.commit();
		if (result)
		{
			LOG_DEBUG("Database transaction committed");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to commit transaction: " % threadData.database.lastError().text() });
		}
		m_dataLock.unlock();
		return result;
	}

	bool DatabaseExecutor::rollbackTransaction()
	{
		auto& threadData = getThreadLocalData();
		bool result = threadData.database.rollback();
		if (result)
		{
			LOG_WARN("Database transaction rolled back");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to rollback transaction: " % threadData.database.lastError().text() });
		}
		m_dataLock.unlock();
		return result;
	}

	void DatabaseExecutor::endTransaction()
	{
		m_dataLock.unlock();
		LOG_DEBUG("Database transaction ended (lock released)");
	}

	void DatabaseExecutor::setDatabasePath(const QString& path)
	{
		QWriteLocker locker(&m_dataLock);
		auto& threadData = getThreadLocalData();

		// 关闭现有连接
		if (threadData.database.isOpen())
		{
			threadData.database.close();
		}

		// 重置初始化状态
		threadData.isInitialized = false;

		// 设置新路径
		m_databasePath = path;

		// 重新初始化
		initialize();
	}

} // namespace nexusdl::database