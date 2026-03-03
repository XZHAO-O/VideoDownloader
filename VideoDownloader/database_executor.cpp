// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_executor.h"

// Project internal headers
#include "connection_manager.h"

namespace nexusdl::database {

	DatabaseExecutor::DatabaseExecutor() noexcept
	{
	}

	std::expected<void, DatabaseError> DatabaseExecutor::executeWrite(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);

		for (auto [key, value] : params.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!execWithLock(query))
		{
			LOG_ERROR(QString{ "executeWrite(QVariantMap) Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return {};
	}

	std::expected<QSqlQuery, DatabaseError> DatabaseExecutor::executeQuery(const QString& connectionNamePrefix, const QString& queryStr, const QVariantMap& params)
	{
		QSqlQuery query = prepareQuery(connectionNamePrefix, queryStr);

		for (auto [key, value] : params.asKeyValueRange())
		{
			query.bindValue(key, value);
		}

		if (!query.exec())
		{
			LOG_ERROR(QString{ "executeQuery(QVariantMap) Failed: " % query.lastError().text() });
			LOG_DEBUG(QString{ "Failed Query: " % queryStr });
			return std::unexpected{ DatabaseError::ExecuteQueryError };
		}

		return query;
	}

	QSqlQuery DatabaseExecutor::prepareQuery(const QString& connectionNamePrefix, const QString& queryStr)
	{
		QSqlQuery query{ ConnectionManager::instance().getConnection(connectionNamePrefix) };
		query.setForwardOnly(true);
		// if prepare error, it will be handled in the caller
		query.prepare(queryStr);
		return query;
	}

	bool DatabaseExecutor::execWithLock(QSqlQuery& query)
	{
		QMutexLocker locker{ &m_writeMutex };
		return query.exec();
	}

	bool DatabaseExecutor::execBatchWithLock(QSqlQuery& query)
	{
		QMutexLocker locker{ &m_writeMutex };
		return query.execBatch();
	}
} // namespace nexusdl::database