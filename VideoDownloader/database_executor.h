// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <expected>
#include <type_traits>

// Qt headers
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QList>
#include <QMutex>

// Project internal headers
#include "database_error.h"
#include "logger.h"

namespace nexusdl::database {

	template<typename T>
	concept IsSequentialContainer = requires {
		typename T::value_type;
		requires std::random_access_iterator<decltype(std::declval<T>().begin())>; // 迭代器是随机访问
	};

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

		// SQL 执行（写操作）- QVariantMap 版本
		std::expected<void, DatabaseError> executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params);

		// SQL 执行（写操作）- 通用顺序容器版本
		template<typename Container>
			requires IsSequentialContainer<Container>
		std::expected<void, DatabaseError> executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const Container& params);

		// SQL 查询并返回 QSqlQuery（读操作）- QVariantMap 版本
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params);

		// SQL 查询并返回 QSqlQuery（读操作）- 通用顺序容器版本
		template<typename Container>
			requires IsSequentialContainer<Container>
		std::expected<QSqlQuery, DatabaseError> executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const Container& params);

		// 批量写操作 - 通用批处理版本（内层为 QVariantMap）
		template<typename BatchContainer>
			requires std::is_same_v<typename BatchContainer::value_type, QVariantMap>
		std::expected<void, DatabaseError> executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const BatchContainer& batchParams);

		// 批量写操作 - 通用批处理版本（内层为顺序容器）
		template<typename BatchContainer>
			requires IsSequentialContainer<typename BatchContainer::value_type>
		std::expected<void, DatabaseError> executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const BatchContainer& batchParams);

	private:
		QSqlQuery prepareQuery(const QString& connectionNamePrefix, const QString& queryStr);

		bool execWithLock(QSqlQuery& query);
		bool execBatchWithLock(QSqlQuery& query);

	private:
		QMutex m_writeMutex;
	};

	template<typename Container>
		requires IsSequentialContainer<Container>
	std::expected<void, DatabaseError> DatabaseExecutor::executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const Container& params)
	{
		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);

		for (int i = 0; const auto & val : params)
		{
			query.bindValue(i, val);
			++i;
		}

		if (!execWithLock(query))
		{
			LOG_ERROR(QString{ "executeWrite Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	template<typename Container>
		requires IsSequentialContainer<Container>
	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const Container& params)
	{
		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);

		for (int i = 0; const auto & val : params)
		{
			query.bindValue(i, val);
			++i;
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "executeQuery Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}
		return query;
	}

	// 内层为 QVariantMap：按名称绑定
	template<typename BatchContainer>
		requires std::is_same_v<typename BatchContainer::value_type, QVariantMap>
	std::expected<void, DatabaseError> DatabaseExecutor::executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const BatchContainer& batchParams)
	{
		QHash<QString, QVariantList> columnValues{};
		columnValues.reserve(batchParams.size() * batchParams[0].size());
		for (const auto& params : batchParams)
		{
			for (auto [key, value] : params.asKeyValueRange())
			{
				columnValues[key].append(value);
			}
		}

		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);
		for (auto [key, value] : columnValues.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!execBatchWithLock(query))
		{
			LOG_ERROR(QString{ "executeWriteBatch Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}
		return {};
	}

	// 内层为顺序容器：按位置绑定
	template<typename BatchContainer>
		requires IsSequentialContainer<typename BatchContainer::value_type>
	std::expected<void, DatabaseError> DatabaseExecutor::executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const BatchContainer& batchParams)
	{
		int numPlaceholders = batchParams[0].size();

		QList<QVariantList> columnValues{};
		columnValues.reserve(numPlaceholders);
		for (int i = 0; i < numPlaceholders; ++i)
		{
			columnValues[i].reserve(batchParams.size());
		}

		for (const auto& params : batchParams)
		{
			for (int i = 0; i < params.size(); ++i)
			{
				columnValues[i].append(params[i]);
			}
		}

		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);
		for (int i = 0; i < numPlaceholders; ++i)
		{
			query.bindValue(i, columnValues[i]);
		}

		if (!execBatchWithLock(query))
		{
			LOG_ERROR(QString{ "executeWriteBatch Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}
		return {};
	}

} // namespace nexusdl::database