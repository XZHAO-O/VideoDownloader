// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <expected>

// Qt headers
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QList>

// Project internal headers
#include "database_error.h"

namespace nexusdl::database {

	class DatabaseExecutor
	{
	public:
		explicit DatabaseExecutor();
		~DatabaseExecutor() = default;

		// 禁用拷贝/移动
		DatabaseExecutor(const DatabaseExecutor&) = delete;
		DatabaseExecutor& operator=(const DatabaseExecutor&) = delete;
		DatabaseExecutor(DatabaseExecutor&&) = delete;
		DatabaseExecutor& operator=(DatabaseExecutor&&) = delete;

		// SQL 执行（写操作）
		std::expected<void, DatabaseError> executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params);
		std::expected<void, DatabaseError> executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params = QVariantList());

		// SQL 查询并返回 QSqlQuery（读操作）
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params);
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params = QVariantList());

		// 批量写操作
		std::expected<void, DatabaseError> executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantMap>& batchParams);
		std::expected<void, DatabaseError> executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantList>& batchParams);

		// 批量查询操作（返回多个结果集）
		std::expected<QList<QSqlQuery>, DatabaseError> executeQueryBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantMap>& batchParams);
		std::expected<QList<QSqlQuery>, DatabaseError> executeQueryBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantList>& batchParams);
	};

} // namespace nexusdl::database