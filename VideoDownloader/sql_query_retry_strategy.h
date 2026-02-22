// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QSqlError>

// Project internal headers
#include "retry_strategy.h"

namespace nexusdl::database {

	class SqlQueryRetryStrategy : public RetryStrategy<SqlQueryRetryStrategy, QSqlError>
	{
	public:
		SqlQueryRetryStrategy()
			: RetryStrategy<SqlQueryRetryStrategy, QSqlError>{ 0,0 }
		{

		}

		SqlQueryRetryStrategy(int maxRetries, int delayMs)
			: RetryStrategy<SqlQueryRetryStrategy, QSqlError>{ maxRetries, delayMs }
		{

		}

		~SqlQueryRetryStrategy() = default;

		SqlQueryRetryStrategy(const SqlQueryRetryStrategy&) = delete;
		SqlQueryRetryStrategy& operator=(const SqlQueryRetryStrategy&) = delete;
		SqlQueryRetryStrategy(SqlQueryRetryStrategy&&) = delete;
		SqlQueryRetryStrategy& operator=(SqlQueryRetryStrategy&&) = delete;

		bool shouldRetryImpl(int attempt, const QSqlError& error)
		{
			if (attempt >= m_maxRetries)
				return false;

			switch (error.type())
			{
			case QSqlError::UnknownError:
				return false;
			default:
				return false;
			}
		}
	};

} // namespace nexusdl::database