// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

namespace nexusdl::log {

	enum class LogLevel
	{
		Trace = 0,
		Debug = 1,
		Info = 2,
		Warn = 3,
		Error = 4,
		Fatal = 5
	};

	inline constexpr const char* levelToString(LogLevel level) noexcept
	{
		switch (level)
		{
		case LogLevel::Trace: return "TRACE";
		case LogLevel::Debug: return "DEBUG";
		case LogLevel::Info: return "INFO";
		case LogLevel::Warn: return "WARN";
		case LogLevel::Error: return "ERROR";
		case LogLevel::Fatal: return "FATAL";
		default: return "UNKNOWN";
		}
	}

}