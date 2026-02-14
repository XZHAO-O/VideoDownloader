// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "conditional_lock.h"

namespace nexusdl::database {

	ConditionalWriteLock::ConditionalWriteLock(QReadWriteLock* lock, bool condition)
		: m_lock{ lock }
		, m_locked{ false }
	{
		if (!condition)
		{
			m_lock->lockForWrite();
			m_locked = true;
		}
	}

	ConditionalWriteLock::~ConditionalWriteLock()
	{
		if (m_locked)
		{
			m_lock->unlock();
		}
	}

	ConditionalReadLock::ConditionalReadLock(QReadWriteLock* lock, bool condition)
		: m_lock{ lock }
		, m_locked{ false }
	{
		if (!condition)
		{
			m_lock->lockForRead();
			m_locked = true;
		}
	}

	ConditionalReadLock::~ConditionalReadLock()
	{
		if (m_locked)
		{
			m_lock->unlock();
		}
	}

} // namespace nexusdl::database