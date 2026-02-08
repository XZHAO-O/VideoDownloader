// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>

namespace nexusdl::log {

	class LogBuffer
	{
	public:
		explicit LogBuffer();
		~LogBuffer() = default;

		LogBuffer(const LogBuffer&) = delete;
		LogBuffer& operator=(const LogBuffer&) = delete;
		LogBuffer(LogBuffer&&) = delete;
		LogBuffer& operator=(LogBuffer&&) = delete;

		void append(const char* data, qint64 size);

		bool isEmpty() const noexcept;

		const QByteArray& data() const noexcept;

		void clear();

		qint64 size() const noexcept;

		bool shouldFlush() const noexcept;

		qint64 writableBytes() const noexcept;

	private:
		QByteArray m_data;
	};

}