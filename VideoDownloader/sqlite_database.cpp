// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sqlite_database.h"

// Qt Core
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>

// Project internal headers
#include "logger.h"
#include "conditional_lock.h"

namespace nexusdl::database {

	thread_local bool SQLiteDatabase::s_inTransaction{ false };

	SQLiteDatabase::SQLiteDatabase(const QString& databaseName, QObject* parent)
		: QObject{ parent }
		, m_dataLock{}
		, m_databaseDirPath{ QCoreApplication::applicationDirPath() % "/database" }
		, m_databaseName{ databaseName }
		, m_fullDatabasePath{ m_databaseDirPath % "/" % m_databaseName }
		, m_connectionNamePrefix{ "SQLiteDatabaseConnection_" % m_databaseName % "_" }
	{
		initialize();
	}

	bool SQLiteDatabase::initialize()
	{
		// 初始化数据库连接
		if (auto result = initConnection(); !result.has_value())
		{
			return false;
		}

		LOG_INFO(QString{ "SQLite database initialized successfully at: " % m_fullDatabasePath });
		return true;
	}

	std::expected<void, DatabaseError> SQLiteDatabase::initConnection()
	{
		return ConnectionManager::instance().initializeConnection(m_databaseName, m_databaseDirPath, m_fullDatabasePath);
	}

	bool SQLiteDatabase::ensureConnection()
	{
		return initialize();
	}

	bool SQLiteDatabase::executeQuery(const QString& queryStr, const QVariantMap& params)
	{
		if (!ensureConnection())
		{
			return false;
		}
		ConditionalWriteLock lock(&m_dataLock, s_inTransaction);

		if (!ConnectionManager::instance().isConnectionInitialized(m_connectionNamePrefix))
		{
			LOG_ERROR("SQLite database is not open");
			return false;
		}

		QSqlQuery query{ ConnectionManager::instance().getConnection(m_connectionNamePrefix) };
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

	bool SQLiteDatabase::executeQuery(const QString& queryStr, const QVariantList& params)
	{
		if (!ensureConnection())
		{
			return false;
		}
		ConditionalWriteLock lock(&m_dataLock, s_inTransaction);

		if (!ConnectionManager::instance().isConnectionInitialized(m_connectionNamePrefix))
		{
			LOG_ERROR("SQLite database is not open");
			return false;
		}

		QSqlQuery query{ ConnectionManager::instance().getConnection(m_connectionNamePrefix) };
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

	QList<QVariantMap> SQLiteDatabase::executeQueryToMap(const QString& queryStr, const QVariantMap& params)
	{
		if (!ensureConnection())
		{
			return {};
		}
		ConditionalReadLock lock(&m_dataLock, s_inTransaction);

		if (!ConnectionManager::instance().isConnectionInitialized(m_connectionNamePrefix))
		{
			LOG_ERROR("SQLite database is not open");
			return {};
		}

		QList<QVariantMap> result{};

		QSqlQuery query{ ConnectionManager::instance().getConnection(m_connectionNamePrefix) };
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

	QList<QVariantMap> SQLiteDatabase::executeQueryToMap(const QString& queryStr, const QVariantList& params)
	{
		if (!ensureConnection())
		{
			return {};
		}
		ConditionalReadLock lock(&m_dataLock, s_inTransaction);

		if (!ConnectionManager::instance().isConnectionInitialized(m_connectionNamePrefix))
		{
			LOG_ERROR("SQLite database is not open");
			return {};
		}

		QList<QVariantMap> result{};

		QSqlQuery query{ ConnectionManager::instance().getConnection(m_connectionNamePrefix) };
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

	QString SQLiteDatabase::lastError() const
	{
		return ConnectionManager::instance().lastError(m_connectionNamePrefix);
	}

	QString SQLiteDatabase::databaseDirPath() const
	{
		return m_databaseDirPath;
	}

	QString SQLiteDatabase::databaseName() const
	{
		return m_databaseName;
	}

	QString SQLiteDatabase::fullPath() const
	{
		return m_fullDatabasePath;
	}

	qint64 SQLiteDatabase::databaseSize() const
	{
		if (QFileInfo fileInfo(m_fullDatabasePath); fileInfo.exists())
		{
			return fileInfo.size();
		}
		else
		{
			return -1;
		}
	}

	bool SQLiteDatabase::beginTransaction()
	{
		m_dataLock.lockForWrite();
		bool result = ConnectionManager::instance().beginTransaction(m_connectionNamePrefix);
		if (result)
		{
			s_inTransaction = true;  // 设置线程局部标志
		}
		else
		{
			m_dataLock.unlock();  // 失败则释放锁
		}
		return result;
	}

	bool SQLiteDatabase::commitTransaction()
	{
		bool result = ConnectionManager::instance().commitTransaction(m_connectionNamePrefix);
		if (s_inTransaction)
		{
			s_inTransaction = false;
			m_dataLock.unlock();
		}
		return result;
	}

	bool SQLiteDatabase::rollbackTransaction()
	{
		bool result = ConnectionManager::instance().rollbackTransaction(m_connectionNamePrefix);
		if (s_inTransaction)
		{
			s_inTransaction = false;
			m_dataLock.unlock();
		}
		return result;
	}

} // namespace nexusdl::database