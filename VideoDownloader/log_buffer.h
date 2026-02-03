// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <QByteArray>

namespace nexusdl::log {

	class LogBuffer
	{
	public:
		static constexpr qint64 kBufferSize = 4 * 1024 * 1024;

		explicit LogBuffer();
		~LogBuffer() = default;

		LogBuffer(const LogBuffer&) = delete;
		LogBuffer& operator=(const LogBuffer&) = delete;
		LogBuffer(LogBuffer&&) = delete;
		LogBuffer& operator=(LogBuffer&&) = delete;

		void append(QByteArray&& message) noexcept;

		bool isEmpty() const;

		const QByteArray& data() const;

		void clear();

		qint64 size() const;

		qint64 writableBytes() const;

	private:
		QByteArray m_data;
	};

}