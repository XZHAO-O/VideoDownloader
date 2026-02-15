// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_executor.h"

// Qt headers
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

// Project internal headers
#include "logger.h"
#include "connection_manager.h"

namespace nexusdl::database {

	DatabaseExecutor::DatabaseExecutor()
	{
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected(DatabaseError::ExecuteQueryError);
		}

		return {};
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params)
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
			return std::unexpected(DatabaseError::ExecuteQueryError);
		}

		return {};
	}

	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQueryToMap(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "Failed to execute select query: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected(DatabaseError::ExecuteQueryError);
		}

		return query;
	}

	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQueryToMap(const QString& connectionNamePrefix, const QString& queryStr, const QVariantList& params)
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
			return std::unexpected(DatabaseError::ExecuteQueryError);
		}

		return query;
	}

} // namespace nexusdl::database