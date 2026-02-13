// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QMutex>
#include <QObject>

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
		std::expected<void, DatabaseError> initializeConnection(const QString& databaseName, const QString& databasePath);
		QSqlDatabase& getConnection(const QString& databaseName);
		const QSqlDatabase& getConnection(const QString& databaseName) const;
		QString getConnectionName(const QString& databaseName) const;
		ConnectionContext& getConnectionContext(const QString& databaseName);
		const ConnectionContext& getConnectionContext(const QString& databaseName) const;

		QString getLastError(const QString& databaseName) const;

		// 查询执行
		QSqlQuery createQuery(const QString& databaseName);
		bool executeQuery(const QString& databaseName, QSqlQuery& query);
		QList<QVariantMap> executeQueryToMap(const QString& databaseName, QSqlQuery& query);

		// 事务管理
		bool beginTransaction(const QString& databaseName);
		bool commitTransaction(const QString& databaseName);
		bool rollbackTransaction(const QString& databaseName);

	private:
		explicit ConnectionManager();
		~ConnectionManager() = default;

		// 线程本地连接存储
		static thread_local QHash<QString, ConnectionContext> m_threadConnections;
	};

} // namespace nexusdl::database