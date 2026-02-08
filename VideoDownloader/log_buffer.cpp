// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "log_buffer.h"

namespace {
	constexpr qint64 kBufferSize{ 4 * 1024 * 1024 };
	constexpr qint64 kBufferUsageThreshold{ 4 }; // 1/4缓冲区使用后可触发刷新
}

namespace nexusdl::log {

	LogBuffer::LogBuffer()
		: m_data{}
	{
		m_data.reserve(kBufferSize);
	}

	void LogBuffer::append(const char* data, qint64 size)
	{
		const qint64 currentSize = m_data.size();
		const qint64 newSize = currentSize + size;

		if (m_data.capacity() < newSize)
		{
			m_data.reserve(newSize);
		}

		std::memcpy(m_data.data() + currentSize, data, size);
		m_data.resize(newSize);
	}

	bool LogBuffer::isEmpty() const noexcept
	{
		return m_data.isEmpty();
	}

	const QByteArray& LogBuffer::data() const noexcept
	{
		return m_data;
	}

	void LogBuffer::clear()
	{
		m_data.resize(0);
	}

	qint64 LogBuffer::size() const noexcept
	{
		return m_data.size();
	}

	bool LogBuffer::shouldFlush() const noexcept
	{
		return m_data.size() > kBufferSize / kBufferUsageThreshold;
	}

	qint64 LogBuffer::writableBytes() const noexcept
	{
		return m_data.capacity() - m_data.size();
	}

}