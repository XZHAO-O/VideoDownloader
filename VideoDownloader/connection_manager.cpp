// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "connection_manager.h"

// Qt headers
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
	thread_local QHash<QString, ConnectionContext> ConnectionManager::s_threadConnections{};
	thread_local QString ConnectionManager::s_threadId{};

	ConnectionManager& ConnectionManager::instance()
	{
		static ConnectionManager instance{};
		return instance;
	}

	ConnectionManager::ConnectionManager()
	{
	}

	QString ConnectionManager::getConnectionName(const QString& connectionNamePrefix) const
	{
		if (s_threadId.isEmpty())
		{
			s_threadId = QString::number(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
		}
		return QString{ connectionNamePrefix % s_threadId };
	}

	ConnectionContext& ConnectionManager::getConnectionContext(const QString& connectionNamePrefix)
	{
		return s_threadConnections[getConnectionName(connectionNamePrefix)];
	}

	const ConnectionContext& ConnectionManager::getConnectionContext(const QString& connectionNamePrefix) const
	{
		return s_threadConnections[getConnectionName(connectionNamePrefix)];
	}

	QString ConnectionManager::lastError(const QString& connectionNamePrefix) const
	{
		return getConnectionContext(connectionNamePrefix).lastError();
	}

	bool ConnectionManager::isConnectionInitialized(const QString& connectionNamePrefix) const
	{
		return getConnectionContext(connectionNamePrefix).isInitialized();
	}

	std::expected<void, DatabaseError> ConnectionManager::initializeConnection(const QString& databaseName, const QString& databaseDirPath, const QString& fullPath)
	{
		auto& context = getConnectionContext(databaseName);
		if (context.isInitialized())
		{
			return {};
		}

		const QString connectionName = getConnectionName(databaseName);

		// 委托给上下文完成实际初始化
		return context.initialize(databaseDirPath, fullPath, connectionName);
	}

	QSqlDatabase& ConnectionManager::getConnection(const QString& connectionNamePrefix)
	{
		return getConnectionContext(connectionNamePrefix).connection();
	}

	const QSqlDatabase& ConnectionManager::getConnection(const QString& connectionNamePrefix) const
	{
		return getConnectionContext(connectionNamePrefix).connection();
	}

	bool ConnectionManager::beginTransaction(const QString& connectionNamePrefix)
	{
		auto& context = getConnectionContext(connectionNamePrefix);
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

	bool ConnectionManager::commitTransaction(const QString& connectionNamePrefix)
	{
		auto& context = getConnectionContext(connectionNamePrefix);
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

	bool ConnectionManager::rollbackTransaction(const QString& connectionNamePrefix)
	{
		auto& context = getConnectionContext(connectionNamePrefix);
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