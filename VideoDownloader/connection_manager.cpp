// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "connection_manager.h"

// Qt Core
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QThread>

// Project internal headers
#include "logger.h"

namespace nexusdl::database {

	// 静态成员初始化
	thread_local QHash<QString, ConnectionContext> ConnectionManager::m_threadConnections{};

	ConnectionManager& ConnectionManager::instance()
	{
		static ConnectionManager instance{};
		return instance;
	}

	ConnectionManager::ConnectionManager()
	{
	}

	QString ConnectionManager::getConnectionName(const QString& databaseName) const
	{
		return QString{ "SQLiteDatabaseConnection_" % databaseName % "_" % QString::number(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()))) };
	}

	ConnectionContext& ConnectionManager::getConnectionContext(const QString& databaseName)
	{
		return m_threadConnections[getConnectionName(databaseName)];
	}

	const ConnectionContext& ConnectionManager::getConnectionContext(const QString& databaseName) const
	{
		return m_threadConnections[getConnectionName(databaseName)];
	}

	QString ConnectionManager::getLastError(const QString& databaseName) const
	{
		return getConnectionContext(databaseName).lastError();
	}

	bool ConnectionManager::isConnectionInitialized(const QString& databaseName) const
	{
		return getConnectionContext(databaseName).isInitialized();
	}

	std::expected<void, DatabaseError> ConnectionManager::initializeConnection(const QString& databaseName, const QString& databasePath)
	{
		auto& context = getConnectionContext(databaseName);
		if (context.isInitialized())
		{
			return {};
		}

		const QString fullPath = databasePath % "/" % databaseName;
		const QString connectionName = getConnectionName(databaseName);

		// 委托给上下文完成实际初始化
		return context.initialize(databasePath, fullPath, connectionName);
	}

	QSqlDatabase& ConnectionManager::getConnection(const QString& databaseName)
	{
		return getConnectionContext(databaseName).connection();
	}

	const QSqlDatabase& ConnectionManager::getConnection(const QString& databaseName) const
	{
		return getConnectionContext(databaseName).connection();
	}

	QSqlQuery ConnectionManager::createQuery(const QString& databaseName)
	{
		return QSqlQuery{ getConnection(databaseName) };
	}

	bool ConnectionManager::executeQuery(const QString& databaseName, QSqlQuery& query)
	{
		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % query.lastQuery() });
			return false;
		}
		return true;
	}

	QList<QVariantMap> ConnectionManager::executeQueryToMap(const QString& databaseName, QSqlQuery& query)
	{
		QList<QVariantMap> result{};

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

	bool ConnectionManager::beginTransaction(const QString& databaseName)
	{
		auto& context = getConnectionContext(databaseName);
		bool result = context.connection().transaction();
		if (!result)
		{
			LOG_ERROR(QString{ "Failed to begin transaction: " % context.lastError() });
		}
		else
		{
			LOG_DEBUG("SQLite database transaction started");
		}
		return result;
	}

	bool ConnectionManager::commitTransaction(const QString& databaseName)
	{
		auto& context = getConnectionContext(databaseName);
		bool result = context.connection().commit();
		if (result)
		{
			LOG_DEBUG("SQLite database transaction committed");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to commit transaction: " % context.lastError() });
		}
		return result;
	}

	bool ConnectionManager::rollbackTransaction(const QString& databaseName)
	{
		auto& context = getConnectionContext(databaseName);
		bool result = context.connection().rollback();
		if (result)
		{
			LOG_WARN("SQLite database transaction rolled back");
		}
		else
		{
			LOG_ERROR(QString{ "Failed to rollback transaction: " % context.lastError() });
		}
		return result;
	}

} // namespace nexusdl::database