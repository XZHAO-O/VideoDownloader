// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "connection_manager.h"

// C++ standard library
#include <thread>

// Qt headers
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QFile>
#include <QCoreApplication>

// Project internal headers
#include "logger.h"

namespace nexusdl::database {

	ConnectionManager& ConnectionManager::instance() noexcept
	{
		static ConnectionManager instance{};
		return instance;
	}

	ConnectionManager::ConnectionManager() noexcept
	{
	}

	std::unordered_map<QString, ConnectionContext>& ConnectionManager::threadConnections() noexcept
	{
		static thread_local std::unordered_map<QString, ConnectionContext> connections{};
		return connections;
	}

	QString& ConnectionManager::threadId() noexcept
	{
		static thread_local QString id{};
		return id;
	}

	QString ConnectionManager::getConnectionName(const QString& connectionNamePrefix) const
	{
		QString& tid = threadId();
		if (tid.isEmpty())
		{
			tid = QString::number(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
		}
		return QString{ connectionNamePrefix % tid };
	}

	ConnectionContext& ConnectionManager::getConnectionContext(const QString& connectionNamePrefix)
	{
		return threadConnections()[getConnectionName(connectionNamePrefix)];
	}

	const ConnectionContext& ConnectionManager::getConnectionContext(const QString& connectionNamePrefix) const
	{
		return threadConnections()[getConnectionName(connectionNamePrefix)];
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
		if (auto& context = getConnectionContext(databaseName); context.isInitialized())
		{
			return {};
		}
		else
		{
			// 委托给上下文完成实际初始化
			return context.initialize(databaseDirPath, fullPath, getConnectionName(databaseName));
		}
	}

	QSqlDatabase& ConnectionManager::getConnection(const QString& connectionNamePrefix)
	{
		return getConnectionContext(connectionNamePrefix).connection();
	}

	const QSqlDatabase& ConnectionManager::getConnection(const QString& connectionNamePrefix) const
	{
		return getConnectionContext(connectionNamePrefix).connection();
	}

	std::expected<void, DatabaseError>  ConnectionManager::beginTransaction(const QString& connectionNamePrefix)
	{
		if (auto& context = getConnectionContext(connectionNamePrefix); context.connection().transaction())
		{
			LOG_DEBUG("SQLite database transaction started");
			return {};
		}
		else
		{
			LOG_ERROR(QString{ "Failed to begin transaction: " % context.lastError() });
			return std::unexpected{ DatabaseError::TransactionError };
		}
	}

	std::expected<void, DatabaseError>  ConnectionManager::commitTransaction(const QString& connectionNamePrefix)
	{
		if (auto& context = getConnectionContext(connectionNamePrefix); context.connection().commit())
		{
			LOG_DEBUG("SQLite database transaction committed");
			return {};
		}
		else
		{
			LOG_ERROR(QString{ "Failed to commit transaction: " % context.lastError() });
			return std::unexpected{ DatabaseError::TransactionError };
		}
	}

	std::expected<void, DatabaseError>  ConnectionManager::rollbackTransaction(const QString& connectionNamePrefix)
	{
		if (auto& context = getConnectionContext(connectionNamePrefix); context.connection().rollback())
		{
			LOG_WARN("SQLite database transaction rolled back");
			return {};
		}
		else
		{
			LOG_ERROR(QString{ "Failed to rollback transaction: " % context.lastError() });
			return std::unexpected{ DatabaseError::TransactionError };
		}
	}

} // namespace nexusdl::database