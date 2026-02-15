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
#include "connection_manager.h"

namespace nexusdl::database {

	SQLiteDatabase::SQLiteDatabase(const QString& databaseName)
		: m_executor{}
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

	std::expected<void, DatabaseError> SQLiteDatabase::executeQuery(const QString& queryStr, const QVariantMap& params)
	{
		if (auto result = initConnection(); !result.has_value())
		{
			return std::unexpected{ result.error() };
		}

		return m_executor.executeQuery(m_connectionNamePrefix, queryStr, params);
	}

	std::expected<void, DatabaseError> SQLiteDatabase::executeQuery(const QString& queryStr, const QVariantList& params)
	{
		if (auto result = initConnection(); !result.has_value())
		{
			return std::unexpected{ result.error() };
		}

		return m_executor.executeQuery(m_connectionNamePrefix, queryStr, params);
	}

	std::expected<QSqlQuery, DatabaseError> SQLiteDatabase::executeQueryToMap(const QString& queryStr, const QVariantMap& params)
	{
		if (auto result = initConnection(); !result.has_value())
		{
			return std::unexpected{ result.error() };
		}

		return m_executor.executeQueryToMap(m_connectionNamePrefix, queryStr, params);
	}

	std::expected<QSqlQuery, DatabaseError> SQLiteDatabase::executeQueryToMap(const QString& queryStr, const QVariantList& params)
	{
		if (auto result = initConnection(); !result.has_value())
		{
			return std::unexpected{ result.error() };
		}

		return m_executor.executeQueryToMap(m_connectionNamePrefix, queryStr, params);
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
		if (QFileInfo fileInfo{ m_fullDatabasePath }; fileInfo.exists())
		{
			return fileInfo.size();
		}

		return -1;
	}

	bool SQLiteDatabase::beginTransaction()
	{
		return ConnectionManager::instance().beginTransaction(m_connectionNamePrefix);
	}

	bool SQLiteDatabase::commitTransaction()
	{
		return ConnectionManager::instance().commitTransaction(m_connectionNamePrefix);
	}

	bool SQLiteDatabase::rollbackTransaction()
	{
		return ConnectionManager::instance().rollbackTransaction(m_connectionNamePrefix);
	}

} // namespace nexusdl::database