// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <expected>

// Qt headers
#include <QSqlDatabase>
#include <QString>

// Project internal headers
#include "database_error.h"

namespace nexusdl::database {

	class ConnectionContext
	{
	public:
		explicit ConnectionContext() noexcept;
		~ConnectionContext();

		ConnectionContext(const ConnectionContext&) = delete;
		ConnectionContext& operator=(const ConnectionContext&) = delete;
		ConnectionContext(ConnectionContext&&) = default;
		ConnectionContext& operator=(ConnectionContext&&) = default;

		// 初始化连接：创建目录、添加数据库、打开并设置PRAGMA
		std::expected<void, DatabaseError> initialize(const QString& databaseDirPath, const QString& fullPath, const QString& connectionName);

		QSqlDatabase& connection() noexcept;
		const QSqlDatabase& connection() const noexcept;
		QString lastError() const;
		bool isInitialized() const noexcept;

	private:
		QSqlDatabase m_connection;
		bool m_isInitialized;
	};

} // namespace nexusdl::database