// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_executor.h"

// Qt headers
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QHash>

// Project internal headers
#include "logger.h"
#include "connection_manager.h"

namespace nexusdl::database {

	DatabaseExecutor::DatabaseExecutor()
	{
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (auto [key, value] : params.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i)
		{
			query.bindValue(i, params[i]);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (auto [key, value] : params.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute select query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return query;
	}

	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i)
		{
			query.bindValue(i, params[i]);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute select query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return query;
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantMap>& batchParams)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		QHash<QString, QVariantList> columnValues{};
		for (const auto& params : batchParams)
		{
			for (auto [key, value] : params.asKeyValueRange())
			{
				columnValues[key].append(value);
			}
		}

		for (auto [key, value] : columnValues.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!query.execBatch())
		{
			LOG_ERROR(QString{ "Failed to execute batch write: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeWriteBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantList>& batchParams)
	{
		// 确定占位符数量（调用者必须确保所有参数列表长度相同）
		int numPlaceholders = batchParams.first().size();
		QList<QVariantList> columnValues{ numPlaceholders , {} };

		for (const auto& params : batchParams)
		{
			for (int i = 0; i < params.size(); ++i)
			{
				columnValues[i].append(params[i]);
			}
		}

		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (int i = 0; i < numPlaceholders; ++i)
		{
			query.bindValue(i, columnValues[i]);
		}

		if (!query.execBatch())
		{
			LOG_ERROR(QString{ "Failed to execute batch write: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	std::expected<QList<QSqlQuery>, DatabaseError> DatabaseExecutor::executeQueryBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantMap>& batchParams)
	{
		auto& db = ConnectionManager::instance().getConnection(connectionNamePrefix);
		QList<QSqlQuery> results{};
		results.reserve(batchParams.size());

		for (const auto& params : batchParams)
		{
			QSqlQuery query{ db };
			query.prepare(queryStr);

			for (auto [key, value] : params.asKeyValueRange())
			{
				query.bindValue(key, value);
			}

			if (!query.exec())
			{
				LOG_ERROR(QString{ "Failed to execute batch query: " % query.lastError().text() });
				LOG_DEBUG(QString{ "Failed Query: " % queryStr });
				return std::unexpected{ DatabaseError::ExecuteQueryError };
			}

			results.append(query);
		}

		return results;
	}

	std::expected<QList<QSqlQuery>, DatabaseError> DatabaseExecutor::executeQueryBatch(const QString& connectionNamePrefix, const QString& queryStr, const QList<QVariantList>& batchParams)
	{
		auto& db = ConnectionManager::instance().getConnection(connectionNamePrefix);
		QList<QSqlQuery> results{};
		results.reserve(batchParams.size());

		for (const auto& params : batchParams)
		{
			QSqlQuery query{ db };
			query.prepare(queryStr);

			for (int i = 0; i < params.size(); ++i)
			{
				query.bindValue(i, params[i]);
			}

			if (!query.exec())
			{
				LOG_ERROR(QString{ "Failed to execute batch query: " % query.lastError().text() });
				LOG_DEBUG(QString{ "Failed Query: " % queryStr });
				return std::unexpected{ DatabaseError::ExecuteQueryError };
			}

			results.append(query);
		}

		return results;
	}

} // namespace nexusdl::database