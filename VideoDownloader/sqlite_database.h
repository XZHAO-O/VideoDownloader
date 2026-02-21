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

		// SQL执行（写操作）- QVariantMap版本
		std::expected<void, DatabaseError> executeWrite(const QString& queryStr, const QVariantMap& params);

		// SQL执行（写操作）- QVariantList版本
		std::expected<void, DatabaseError> executeWrite(const QString& queryStr, const QVariantList& params = QVariantList());

		// SQL执行（写操作）- 通用顺序容器版本
		template<typename Container>
		std::expected<void, DatabaseError> executeWrite(const QString& queryStr, const Container& params)
		{
			if (auto result = initConnection(); !result.has_value())
			{
				return std::unexpected{ result.error() };
			}
			return m_executor.executeWrite(m_connectionNamePrefix, queryStr, params);
		}

		// SQL查询（读操作）- QVariantMap版本
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& queryStr, const QVariantMap& params);

		// SQL查询（读操作）- QVariantList版本
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());

		// SQL查询（读操作）- 通用顺序容器版本
		template<typename Container>
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& queryStr, const Container& params)
		{
			if (auto result = initConnection(); !result.has_value())
			{
				return std::unexpected{ result.error() };
			}
			return m_executor.executeQuery(m_connectionNamePrefix, queryStr, params);
		}

		// 批量写操作 - 批参数为 QVariantMap 的容器
		template<typename BatchContainer>
			requires std::is_same_v<typename BatchContainer::value_type, QVariantMap>
		std::expected<void, DatabaseError> executeWriteBatch(const QString& queryStr, const BatchContainer& batchParams)
		{
			if (auto result = initConnection(); !result.has_value())
			{
				return std::unexpected{ result.error() };
			}
			return m_executor.executeWriteBatch(m_connectionNamePrefix, queryStr, batchParams);
		}

		// 批量写操作 - 批参数为顺序容器的容器
		template<typename BatchContainer>
		std::expected<void, DatabaseError> executeWriteBatch(const QString& queryStr, const BatchContainer& batchParams)
		{
			if (auto result = initConnection(); !result.has_value())
			{
				return std::unexpected{ result.error() };
			}
			return m_executor.executeWriteBatch(m_connectionNamePrefix, queryStr, batchParams);
		}

		// 实用方法
		QString lastError() const;
		QString databaseDirPath() const;
		QString databaseName() const;
		QString fullPath() const;
		qint64 databaseSize() const;

		// 事务支持
		std::expected<void, DatabaseError> beginTransaction();
		std::expected<void, DatabaseError> commitTransaction();
		std::expected<void, DatabaseError> rollbackTransaction();

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