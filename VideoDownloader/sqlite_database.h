// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QReadWriteLock>
#include <QObject>
#include <QHash>

// Project internal headers
#include "connection_manager.h"

namespace nexusdl::database {

	class SQLiteDatabase : public QObject
	{
		Q_OBJECT

	public:
		explicit SQLiteDatabase(const QString& databaseName, QObject* parent = nullptr);
		~SQLiteDatabase() = default;

		SQLiteDatabase(const SQLiteDatabase&) = delete;
		SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;
		SQLiteDatabase(SQLiteDatabase&&) = delete;
		SQLiteDatabase& operator=(SQLiteDatabase&&) = delete;

		// 初始化数据库
		bool initialize();

		// SQL执行（通用方法）
		bool executeQuery(const QString& queryStr, const QVariantMap& params);
		bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
		QList<QVariantMap> executeQueryToMap(const QString& queryStr, const QVariantMap& params);
		QList<QVariantMap> executeQueryToMap(const QString& queryStr, const QVariantList& params = QVariantList());

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
		bool ensureConnection();

	private:
		mutable QReadWriteLock m_dataLock;
		static thread_local bool s_inTransaction;
		const QString m_databaseDirPath;
		const QString m_databaseName;
		const QString m_fullDatabasePath;
		const QString m_connectionNamePrefix;
	};

} // namespace nexusdl::database