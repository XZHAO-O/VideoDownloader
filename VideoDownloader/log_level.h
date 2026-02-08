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

	[[nodiscard]] inline constexpr const char* levelToString(LogLevel level) noexcept
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

	[[nodiscard]] inline constexpr const char* levelToColoredString(LogLevel level) noexcept
	{
		switch (level)
		{
		case LogLevel::Trace:
			return "\033[90mTRACE\033[0m";  // 灰色
		case LogLevel::Debug:
			return "\033[94mDEBUG\033[0m";  // 亮蓝色
		case LogLevel::Info:
			return "\033[92mINFO\033[0m";   // 亮绿色
		case LogLevel::Warn:
			return "\033[93mWARN\033[0m";   // 亮黄色
		case LogLevel::Error:
			return "\033[91mERROR\033[0m";  // 亮红色
		case LogLevel::Fatal:
			return "\033[41;97;1mFATAL\033[0m";  // 红底白字加粗
		default:
			return "UNKNOWN";
		}
	}

}