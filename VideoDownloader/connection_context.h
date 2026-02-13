// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <QSqlDatabase>
#include <QString>
#include <expected>

#include "database_error.h"   // 引入独立错误枚举

namespace nexusdl::database {

	class ConnectionContext
	{
	public:
		explicit ConnectionContext();
		~ConnectionContext();

		// 初始化连接：创建目录、添加数据库、打开并设置PRAGMA
		std::expected<void, DatabaseError> initialize(const QString& databasePath, const QString& fullPath, const QString& connectionName);

		QSqlDatabase& connection() noexcept;
		const QSqlDatabase& connection() const noexcept;
		QString lastError() const;
		bool isInitialized() const noexcept;

	private:
		QSqlDatabase m_connection;
		bool m_isInitialized;
	};

} // namespace nexusdl::database