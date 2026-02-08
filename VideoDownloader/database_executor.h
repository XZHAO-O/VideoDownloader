// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QReadWriteLock>
#include <QObject>

namespace nexusdl::database {

	class DatabaseExecutor : public QObject
	{
		Q_OBJECT

	public:
		explicit DatabaseExecutor(QObject* parent = nullptr);
		~DatabaseExecutor();

		DatabaseExecutor(const DatabaseExecutor&) = delete;
		DatabaseExecutor& operator=(const DatabaseExecutor&) = delete;
		DatabaseExecutor(DatabaseExecutor&&) = delete;
		DatabaseExecutor& operator=(DatabaseExecutor&&) = delete;

		// 初始化数据库（每个线程需要单独调用）
		bool initialize();

		// SQL执行（通用方法）
		bool executeQuery(const QString& queryStr, const QVariantMap& params);
		bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
		QList<QVariantMap> executeQueryToMap(const QString& queryStr, const QVariantMap& params);
		QList<QVariantMap> executeQueryToMap(const QString& queryStr, const QVariantList& params = QVariantList());

		// 实用方法
		QString lastError() const;
		QString databasePath() const;
		qint64 databaseSize() const;

		// 事务支持
		bool beginTransaction();
		bool commitTransaction();
		bool rollbackTransaction();
		void endTransaction();  // 释放写锁

	private:
		bool initConnection();
		bool ensureConnection();

	private:
		static thread_local QSqlDatabase m_database;
		const QString m_databasePath;
		mutable QReadWriteLock m_rwLock;
		static thread_local bool m_isInitialized;
	};

} // namespace nexusdl::database