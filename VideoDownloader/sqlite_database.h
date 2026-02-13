// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QReadWriteLock>
#include <QObject>
#include <QHash>

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
		QString databasePath() const;
		QString databaseName() const;
		QString fullDatabasePath() const;
		qint64 databaseSize() const;

		// 事务支持
		bool beginTransaction();
		bool commitTransaction();
		bool rollbackTransaction();
		void endTransaction();

	private:
		struct ConnectionContext
		{
			QSqlDatabase connection{};
			bool isInitialized{ false };

			~ConnectionContext()
			{
				if (connection.isOpen())
				{
					connection.close();
				}
			}
		};

		bool initConnection();
		bool ensureConnection();

		// 获取连接名称
		QString getConnectionName() const;

		// 获取当前实例的线程本地数据
		ConnectionContext& getConnectionContext();
		const ConnectionContext& getConnectionContext() const;

	private:
		static thread_local QHash<QString, ConnectionContext> m_threadConnections;
		mutable QReadWriteLock m_dataLock;
		const QString m_databasePath;
		const QString m_databaseName;
	};

} // namespace nexusdl::database