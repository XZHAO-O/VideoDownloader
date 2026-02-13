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

namespace nexusdl::database {

	// 静态thread_local成员初始化
	thread_local QHash<QString, SQLiteDatabase::ConnectionContext> SQLiteDatabase::m_threadConnections;

	SQLiteDatabase::SQLiteDatabase(const QString& databaseName, QObject* parent)
		: QObject{ parent }
		, m_dataLock{}
		, m_databasePath{ QCoreApplication::applicationDirPath() % "/database" }
		, m_databaseName{ databaseName }
	{
		initialize();
	}

	bool SQLiteDatabase::initialize()
	{
		// 初始化数据库连接
		if (!initConnection())
		{
			return false;
		}

		LOG_INFO(QString{ "SQLite database initialized successfully at: " % fullDatabasePath() });
		return true;
	}

	QString SQLiteDatabase::getConnectionName() const
	{
		return QString{ "SQLiteDatabaseConnection_" % m_databaseName % "_" %
			QString::number(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()))) };
	}

	bool SQLiteDatabase::initConnection()
	{
		QWriteLocker locker(&m_dataLock);

		auto& context = getConnectionContext();

		if (context.isInitialized)
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

		const QString fullPath = fullDatabasePath();
		const bool exists = QFile::exists(fullPath);

		const QString connectionName = getConnectionName();

		// 如果连接已存在，先移除
		if (QSqlDatabase::contains(connectionName))
		{
			QSqlDatabase::removeDatabase(connectionName);
		}

		context.connection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
		context.connection.setDatabaseName(fullPath);

		if (!context.connection.open())
		{
			LOG_ERROR(QString{ "Failed to open SQLite database: " % context.connection.lastError().text() });
			return false;
		}

		// 如果是新创建的数据库，设置SQLite参数
		if (!exists)
		{
			QSqlQuery query{ context.connection };
			query.exec("PRAGMA foreign_keys = ON");
			query.exec("PRAGMA journal_mode = WAL");
			query.exec("PRAGMA synchronous = NORMAL");
			query.exec("PRAGMA cache_size = -64000");
			query.exec("PRAGMA busy_timeout = 5000");

			LOG_DEBUG("SQLite database created with optimized settings");
		}
		else
		{
			LOG_DEBUG("Existing SQLite database opened");
		}

		context.isInitialized = true;

		return true;
	}

	bool SQLiteDatabase::ensureConnection()
	{
		return initialize();
	}

	SQLiteDatabase::ConnectionContext& SQLiteDatabase::getConnectionContext()
	{
		QString key = getConnectionName();
		return m_threadConnections[key];
	}

	const SQLiteDatabase::ConnectionContext& SQLiteDatabase::getConnectionContext() const
	{
		QString key = getConnectionName();
		return m_threadConnections[key];
	}

	bool SQLiteDatabase::executeQuery(const QString& queryStr, const QVariantMap& params)
	{
		QWriteLocker locker{ &m_dataLock };
		auto& context = getConnectionContext();

		if (!context.connection.isOpen())
		{
			LOG_ERROR("SQLite database is not open");
			return false;
		}

		QSqlQuery query{ context.connection };
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
		QWriteLocker locker{ &m_dataLock };
		auto& context = getConnectionContext();

		if (!context.connection.isOpen())
		{
			LOG_ERROR("SQLite database is not open");
			return false;
		}

		QSqlQuery query{ context.connection };
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
		QReadLocker locker{ &m_dataLock };
		auto& context = getConnectionContext();

		QList<QVariantMap> result{};

		if (!context.connection.isOpen())
		{
			LOG_ERROR("SQLite database is not open");
			return result;
		}

		QSqlQuery query{ context.connection };
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
		QReadLocker locker{ &m_dataLock };
		auto& context = getConnectionContext();

		QList<QVariantMap> result{};

		if (!context.connection.isOpen())
		{
			LOG_ERROR("SQLite database is not open");
			return result;
		}

		QSqlQuery query{ context.connection };
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
		QReadLocker locker{ &m_dataLock };
		auto& context = getConnectionContext();
		return context.connection.lastError().text();
	}

	QString SQLiteDatabase::databasePath() const
	{
		return m_databasePath;
	}

	QString SQLiteDatabase::databaseName() const
	{
		return m_databaseName;
	}

	QString SQLiteDatabase::fullDatabasePath() const
	{
		return m_databasePath % "/" % m_databaseName;
	}

	qint64 SQLiteDatabase::databaseSize() const
	{
		if (QFileInfo fileInfo(fullDatabasePath()); fileInfo.exists())
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
		auto& context = getConnectionContext();
		bool result = context.connection.transaction();
		if (!result)
		{
			LOG_ERROR(QString{ "Failed to begin transaction: " % context.connection.lastError().text() });
			m_dataLock.unlock();
		}
		else
		{
			LOG_DEBUG("SQLite database transaction started");
		}
		return result;
	}

	bool SQLiteDatabase::commitTransaction()
	{
		auto& context = getConnectionContext();
		bool result = context.connection.commit();
		if (result)
		{
			LOG_DEBUG("SQLite database transaction committed");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to commit transaction: " % context.connection.lastError().text() });
		}
		m_dataLock.unlock();
		return result;
	}

	bool SQLiteDatabase::rollbackTransaction()
	{
		auto& context = getConnectionContext();
		bool result = context.connection.rollback();
		if (result)
		{
			LOG_WARN("SQLite database transaction rolled back");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to rollback transaction: " % context.connection.lastError().text() });
		}
		m_dataLock.unlock();
		return result;
	}

	void SQLiteDatabase::endTransaction()
	{
		m_dataLock.unlock();
		LOG_DEBUG("SQLite database transaction ended (lock released)");
	}

} // namespace nexusdl::database