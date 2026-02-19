// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

namespace nexusdl::database {

	template<typename Derived, typename ErrorType>
	class RetryStrategy
	{
	public:
		RetryStrategy(int maxRetries, int delayMs)
			: m_maxRetries{ maxRetries }
			, m_delayMs{ delayMs }
		{

		}
		bool shouldRetry(int attempt, const ErrorType& error)
		{
			return static_cast<Derived*>(this)->shouldRetryImpl(attempt, error);
		}

	protected:
		int m_maxRetries;
		int m_delayMs;
	};

} // namespace nexusdl::database