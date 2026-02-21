// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>

// Project internal headers
#include "connection_context.h"

namespace nexusdl::database {

	class ConnectionManager
	{
	public:
		static ConnectionManager& instance();

		// 禁用拷贝/移动
		ConnectionManager(const ConnectionManager&) = delete;
		ConnectionManager& operator=(const ConnectionManager&) = delete;
		ConnectionManager(ConnectionManager&&) = delete;
		ConnectionManager& operator=(ConnectionManager&&) = delete;

		// 连接管理
		bool isConnectionInitialized(const QString& databaseName) const;
		std::expected<void, DatabaseError> initializeConnection(const QString& databaseName, const QString& databaseDirPath, const QString& fullPath);

		QString getConnectionName(const QString& connectionNamePrefix) const;
		ConnectionContext& getConnectionContext(const QString& connectionNamePrefix);
		// 当连接不存在时会自动创建一个默认的连接上下文
		const ConnectionContext& getConnectionContext(const QString& connectionNamePrefix) const;
		QSqlDatabase& getConnection(const QString& connectionNamePrefix);
		const QSqlDatabase& getConnection(const QString& connectionNamePrefix) const;

		QString lastError(const QString& connectionNamePrefix) const;

		// 事务基础操作
		std::expected<void, DatabaseError>  beginTransaction(const QString& connectionNamePrefix);
		std::expected<void, DatabaseError>  commitTransaction(const QString& connectionNamePrefix);
		std::expected<void, DatabaseError>  rollbackTransaction(const QString& connectionNamePrefix);

	private:
		explicit ConnectionManager();
		~ConnectionManager() = default;

		// 线程本地连接存储
		static thread_local QHash<QString, ConnectionContext> s_threadConnections;
		static thread_local QString s_threadId;
	};

} // namespace nexusdl::database