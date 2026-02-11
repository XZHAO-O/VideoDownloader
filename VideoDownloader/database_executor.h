// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt Core
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QReadWriteLock>
#include <QObject>
#include <QSharedPointer>

namespace nexusdl::database {

	class DatabaseExecutor : public QObject
	{
		Q_OBJECT

	public:
		explicit DatabaseExecutor(QObject* parent = nullptr);
		explicit DatabaseExecutor(const QString& databasePath, QObject* parent = nullptr);
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

		// 设置数据库路径
		void setDatabasePath(const QString& path);

	private:
		bool initConnection();
		bool ensureConnection();

		// 每个实例都有自己的数据库连接和初始化状态
		struct ThreadLocalData {
			QSqlDatabase database;
			bool isInitialized{ false };
		};

		mutable QReadWriteLock m_dataLock;
		QString m_databasePath;
		static thread_local std::unordered_map<std::string, ThreadLocalData> s_threadLocalDatabases;

		// 获取当前实例的线程本地数据
		ThreadLocalData& getThreadLocalData();
		const ThreadLocalData& getThreadLocalData() const;
	};

} // namespace nexusdl::database