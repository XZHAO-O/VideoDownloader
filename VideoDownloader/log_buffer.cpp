// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "log_buffer.h"

namespace {
	constexpr qint64 kBufferSize{ 4 * 1024 * 1024 };
	constexpr qint64 kBufferUsageThreshold{ 4 }; // 1/4缓冲区使用后可触发刷新
}

namespace nexusdl::log {

	LogBuffer::LogBuffer()
	{
		m_data.reserve(kBufferSize);
	}

	void LogBuffer::append(QByteArray&& message) noexcept
	{
		m_data.append(std::move(message));
	}

	bool LogBuffer::isEmpty() const
	{
		return m_data.isEmpty();
	}

	const QByteArray& LogBuffer::data() const
	{
		return m_data;
	}

	void LogBuffer::clear()
	{
		m_data.clear();
	}

	qint64 LogBuffer::size() const
	{
		return m_data.size();
	}

	bool LogBuffer::shouldFlush() const
	{
		return m_data.size() > kBufferSize / kBufferUsageThreshold;
	}

	qint64 LogBuffer::writableBytes() const
	{
		return m_data.capacity() - m_data.size();
	}

}