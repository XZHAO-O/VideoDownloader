// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QReadWriteLock>

namespace nexusdl::database {

	class ConditionalWriteLock
	{
	public:
		ConditionalWriteLock(QReadWriteLock* lock, bool condition);
		~ConditionalWriteLock();

		ConditionalWriteLock(const ConditionalWriteLock&) = delete;
		ConditionalWriteLock& operator=(const ConditionalWriteLock&) = delete;

	private:
		QReadWriteLock* m_lock;
		bool m_locked;
	};

	class ConditionalReadLock
	{
	public:
		ConditionalReadLock(QReadWriteLock* lock, bool condition);
		~ConditionalReadLock();

		ConditionalReadLock(const ConditionalReadLock&) = delete;
		ConditionalReadLock& operator=(const ConditionalReadLock&) = delete;

	private:
		QReadWriteLock* m_lock;
		bool m_locked;
	};

} // namespace nexusdl::database