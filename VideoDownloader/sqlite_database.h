// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlQuery>
#include <QString>

// Project internal headers
#include "database_executor.h"

namespace nexusdl::database {

	class SQLiteDatabase
	{
	public:
		explicit SQLiteDatabase(const QString& databaseName);
		~SQLiteDatabase() = default;

		SQLiteDatabase(const SQLiteDatabase&) = delete;
		SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;
		SQLiteDatabase(SQLiteDatabase&&) = delete;
		SQLiteDatabase& operator=(SQLiteDatabase&&) = delete;

		// 初始化数据库
		bool initialize();

		// SQL执行（写操作）
		std::expected<void, DatabaseError> executeWrite(const QString& queryStr, const QVariantMap& params);
		std::expected<void, DatabaseError> executeWrite(const QString& queryStr, const QVariantList& params = QVariantList());

		// SQL查询（读操作）
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& queryStr, const QVariantMap& params);
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());

		// 实用方法
		QString lastError() const;
		QString databaseDirPath() const;
		QString databaseName() const;
		QString fullPath() const;
		qint64 databaseSize() const;

		// 事务支持
		bool beginTransaction();
		bool commitTransaction();
		bool rollbackTransaction();

	private:
		std::expected<void, DatabaseError> initConnection();

	private:
		DatabaseExecutor m_executor;
		const QString m_databaseDirPath;
		const QString m_databaseName;
		const QString m_fullDatabasePath;
		const QString m_connectionNamePrefix;
	};

} // namespace nexusdl::database