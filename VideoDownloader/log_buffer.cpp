// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "log_buffer.h"

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

	qint64 LogBuffer::writableBytes() const
	{
		return m_data.capacity() - m_data.size();
	}

}