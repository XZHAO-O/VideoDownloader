// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <atomic>
#include <chrono>
#include <source_location>

// Qt Core
#include <QDir>
#include <QTimer>
#include <QMutex>
#include <QThread>

// Project internal headers
#include "log_level.h"
#include "log_buffer.h"

#include "macros.h"

namespace nexusdl::log {

	// 检查是否为字符串字面量（const char[N]）或QString
	template<typename T>
	concept StringLiteralOrQString = requires
	{
		requires (std::is_array_v<std::remove_cvref_t<T>>&&
	std::is_same_v<std::remove_extent_t<std::remove_cvref_t<T>>, char>) ||
		std::is_same_v<std::remove_cvref_t<T>, QString>;
	};

	class Logger : public QObject
	{
		Q_OBJECT

	public:
		static Logger& instance();

		bool start();

		bool stop();

		void setLogLevel(LogLevel level);

		bool shouldLog(LogLevel level) const;

		// 核心日志方法 - 声明
		template<typename StringType>
			requires StringLiteralOrQString<StringType>
		void log(LogLevel level, StringType&& message,
			const std::source_location& location = std::source_location::current());

		// 日志方法
		template<typename StringType>
		void trace(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Trace, std::forward<StringType>(message), location);
		}

		template<typename StringType>
		void debug(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Debug, std::forward<StringType>(message), location);
		}

		template<typename StringType>
		void info(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Info, std::forward<StringType>(message), location);
		}

		template<typename StringType>
		void warn(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Warn, std::forward<StringType>(message), location);
		}

		template<typename StringType>
		void error(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Error, std::forward<StringType>(message), location);
		}

		template<typename StringType>
		void fatal(StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			log(LogLevel::Fatal, std::forward<StringType>(message), location);
		}

		// 日志旋转和归档
		void setRotation(qint64 maxSize, qint64 maxFiles);

		// 设置定时写入间隔（毫秒）
		void setFlushInterval(int milliseconds);

	private slots:
		void onFlushTimer();

	private:
		explicit Logger(QObject* parent = nullptr);
		~Logger();

		// 禁止拷贝和移动
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		// 文件管理
		bool openLogFile();
		bool rotateIfNeeded();
		void cleanupOldFiles();

		bool writeToBuffer(QByteArray&& message);
		bool writeBufferToFile();

		// 工具函数
		QString getTimeStamp() const;
		uint32_t getCurrentProcessId() const;
		QString levelToColor(LogLevel level) const;
		void getLogBasicInfo(const std::source_location& location,
			std::tm& tm, int& milliseconds,
			const char*& file, const char*& shortFile,
			int& line, QString& threadName, bool& hasThreadName);

		// 输出模式函数
		template<typename StringType>
		void writeToVSDebug(LogLevel level, StringType&& message,
			const std::tm& tm, int milliseconds,
			const char* file, int line,
			const QString& threadName, bool hasThreadName);

		template<typename StringType>
		void writeToConsole(LogLevel level, StringType&& message,
			const std::tm& tm, int milliseconds,
			const char* shortFile, int line,
			const QString& threadName, bool hasThreadName);

		template<typename StringType>
		void writeToFile(LogLevel level, StringType&& message,
			const std::tm& tm, int milliseconds,
			const char* shortFile, int line,
			const QString& threadName, bool hasThreadName);

		void initialize();
		void shutdown();

	private:
		// 核心配置
		std::atomic<LogLevel> m_logLevel;
		std::atomic<bool> m_initialized;

		// 文件相关
		const QString m_logDir;
		QFile m_logFile;
		QString m_currentLogPath;
		qint64 m_currentFileSize;

		// 日志轮转配置
		std::atomic<qint64> m_maxFileSize;
		std::atomic<qint64> m_maxFiles;

		// 缓冲区管理
		std::unique_ptr<LogBuffer> m_currentBuffer;
		std::unique_ptr<LogBuffer> m_nextBuffer;
		QMutex m_mutex;

		// 定时刷新
		QTimer m_flushTimer;
		int m_flushInterval;

		// 统计信息
		qint64 m_lastFlushTime;
		qint64 m_logsSinceLastFlush;
	};

	// =============== 私有输出函数的实现 ===============

	template<typename StringType>
	void Logger::writeToVSDebug(LogLevel level, StringType&& message,
		const std::tm& tm, int milliseconds,
		const char* file, int line,
		const QString& threadName, bool hasThreadName)
	{
		QString consoleOutput;

		char timeBuffer[68];
		size_t len = std::strftime(timeBuffer, sizeof(timeBuffer) - 4,
			"%Y-%m-%d %H:%M:%S", &tm);
		std::snprintf(timeBuffer + len, 5, ".%03d", milliseconds);

		QString debugBasePart;
		if (hasThreadName)
		{
			debugBasePart = QString("[%1] [%2] [%3]")
				.arg(timeBuffer)
				.arg(levelToString(level))
				.arg(threadName);
		}
		else
		{
			uint32_t pid = getCurrentProcessId();
			debugBasePart = QString("[%1] [%2] [%3]")
				.arg(timeBuffer)
				.arg(levelToString(level))
				.arg(pid);
		}

		// 使用VS格式，双击跳转
		consoleOutput = QString("%1\n%2(%3): %4\n")
			.arg(debugBasePart)
			.arg(QDir::toNativeSeparators(QString::fromUtf8(file)))
			.arg(line)
			.arg(std::forward<StringType>(message));

		qDebug().noquote() << consoleOutput;
	}

	template<typename StringType>
	void Logger::writeToConsole(LogLevel level, StringType&& message,
		const std::tm& tm, int milliseconds,
		const char* shortFile, int line,
		const QString& threadName, bool hasThreadName)
	{
		QString consoleOutput;

		char timeBuffer[68];
		size_t len = std::strftime(timeBuffer, sizeof(timeBuffer) - 4,
			"%Y-%m-%d %H:%M:%S", &tm);
		std::snprintf(timeBuffer + len, 5, ".%03d", milliseconds);

		QString debugBasePart;
		if (hasThreadName)
		{
			debugBasePart = QString("[%1] [%2] [%3]")
				.arg(timeBuffer)
				.arg(levelToString(level))
				.arg(threadName);
		}
		else
		{
			uint32_t pid = getCurrentProcessId();
			debugBasePart = QString("[%1] [%2] [%3]")
				.arg(timeBuffer)
				.arg(levelToString(level))
				.arg(pid);
		}

		// 带颜色的控制台输出
		QString color = levelToColor(level);
		QString resetColor = "\033[0m";

		consoleOutput = QString("%1%2 %3(%4): %5%6")
			.arg(color)
			.arg(debugBasePart)
			.arg(shortFile)
			.arg(line)
			.arg(std::forward<StringType>(message))
			.arg(resetColor);

		qDebug().noquote() << consoleOutput;
	}

	// 辅助函数：将整数转换为字符串并更新指针
	template<typename T>
	inline char* writeNumber(char* ptr, T value) {
		auto [end, ec] = std::to_chars(ptr, ptr + 32, value);
		return end;
	}

	// 辅助函数：复制字符串并更新指针
	inline char* writeString(char* ptr, const char* str) {
		size_t len = strlen(str);
		memcpy(ptr, str, len);
		return ptr + len;
	}

	// 辅助函数：补零格式化两位数字
	inline char* writeTwoDigits(char* ptr, int value) {
		if (value < 10) {
			*ptr++ = '0';
		}
		return writeNumber(ptr, value);
	}

	// 辅助函数：补零格式化三位毫秒
	inline char* writeThreeDigits(char* ptr, int value) {
		if (value < 100) *ptr++ = '0';
		if (value < 10) *ptr++ = '0';
		return writeNumber(ptr, value);
	}

	template<typename StringType>
	void Logger::writeToFile(LogLevel level, StringType&& message,
		const std::tm& tm, int milliseconds,
		const char* shortFile, int line,
		const QString& threadName, bool hasThreadName)
	{
		static thread_local char buffer[1024];  // 足够大的缓冲区

		char* ptr = buffer;

		// 开始构建日志行
		*ptr++ = '[';

		// 年份 (4位)
		ptr = writeNumber(ptr, tm.tm_year + 1900);
		*ptr++ = '-';

		// 月份 (补零)
		ptr = writeTwoDigits(ptr, tm.tm_mon + 1);
		*ptr++ = '-';

		// 日期 (补零)
		ptr = writeTwoDigits(ptr, tm.tm_mday);
		*ptr++ = ' ';

		// 小时 (补零)
		ptr = writeTwoDigits(ptr, tm.tm_hour);
		*ptr++ = ':';

		// 分钟 (补零)
		ptr = writeTwoDigits(ptr, tm.tm_min);
		*ptr++ = ':';

		// 秒 (补零)
		ptr = writeTwoDigits(ptr, tm.tm_sec);
		*ptr++ = '.';

		// 毫秒 (补零到3位)
		ptr = writeThreeDigits(ptr, milliseconds);

		// 日志等级
		ptr = writeString(ptr, "] [");
		ptr = writeString(ptr, levelToString(level));
		ptr = writeString(ptr, "] [");

		if (hasThreadName)
		{
			// 使用线程名
			QByteArray utf8ThreadName = threadName.toUtf8();
			const char* threadNameStr = utf8ThreadName.constData();
			size_t threadNameLen = utf8ThreadName.size();
			memcpy(ptr, threadNameStr, threadNameLen);
			ptr += threadNameLen;
		}
		else
		{
			// 使用进程ID - 直接格式化为整数
			ptr = writeNumber(ptr, getCurrentProcessId());
		}

		// 进程ID
		ptr = writeString(ptr, "] ");

		// 文件名
		ptr = writeString(ptr, shortFile);
		*ptr++ = '(';

		// 行号
		ptr = writeNumber(ptr, line);
		ptr = writeString(ptr, "): ");
		if constexpr (std::is_same_v<std::decay_t<StringType>, QString>)
		{
			// 预转换消息为UTF-8（如果消息固定可移出循环）
			QByteArray utf8Message = message.toUtf8();
			const char* messageStr = utf8Message.constData();
			size_t messageLen = utf8Message.size();
			// 消息内容
			memcpy(ptr, messageStr, messageLen);
			ptr += messageLen;
		}
		else
		{
			size_t messageLen = std::strlen(message);
			memcpy(ptr, message, messageLen);
			ptr += messageLen;
		}
		// 换行
		*ptr++ = '\n';

		// 现在buffer[0]到ptr-1包含了完整的日志行
		// 可以直接使用或转换为QString/QByteArray
		QByteArray logData = QByteArray(buffer, static_cast<int>(ptr - buffer));

		// 写入缓冲区
		writeToBuffer(std::move(logData));
	}

	// =============== 模式1: VSOUTPUT_MODE ===============
	// 仅输出到VS调试控制台
	#ifdef LOG_VSOUTPUT_MODE

	template<typename StringType>
		requires StringLiteralOrQString<StringType>
	void Logger::log(LogLevel level, StringType&& message,
		const std::source_location& location)
	{
		std::tm tm;
		int milliseconds;
		const char* file;
		const char* shortFile;
		int line;
		QString threadName;
		bool hasThreadName;

		getLogBasicInfo(location, tm, milliseconds, file, shortFile, line, threadName, hasThreadName);

		#ifdef QT_DEBUG
		// 调试模式：输出到VS控制台
		writeToVSDebug(level, std::forward<StringType>(message),
			tm, milliseconds,
			file, line, threadName, hasThreadName);
		#endif // QT_DEBUG
	}

	// =============== 模式2: CONSOLEOUTPUT_MODE ===============
	// 仅输出到彩色控制台
	#elif defined(LOG_CONSOLEOUTPUT_MODE)

	template<typename StringType>
		requires StringLiteralOrQString<StringType>
	void Logger::log(LogLevel level, StringType&& message,
		const std::source_location& location)
	{
		std::tm tm;
		int milliseconds;
		const char* file;
		const char* shortFile;
		int line;
		QString threadName;
		bool hasThreadName;

		getLogBasicInfo(location, tm, milliseconds, file, shortFile, line, threadName, hasThreadName);

		#ifdef QT_DEBUG
		// 调试模式：输出到控制台
		writeToConsole(level, std::forward<StringType>(message),
			tm, milliseconds,
			shortFile, line, threadName, hasThreadName);
		#endif // QT_DEBUG
	}

	// =============== 模式3: DEBUG_MODE ===============
	// 调试模式：同时输出到控制台和文件
	#elif defined(LOG_DEBUG_MODE)

	template<typename StringType>
		requires StringLiteralOrQString<StringType>
	void Logger::log(LogLevel level, StringType&& message,
		const std::source_location& location)
	{
		std::tm tm;
		int milliseconds;
		const char* file;
		const char* shortFile;
		int line;
		QString threadName;
		bool hasThreadName;

		getLogBasicInfo(location, tm, milliseconds, file, shortFile, line, threadName, hasThreadName);

		// 调试模式：同时输出到控制台和文件
		#ifdef Q_OS_WIN
		writeToVSDebug(level, std::forward<StringType>(message),
			tm, milliseconds,
			file, line, threadName, hasThreadName);
		#else
		writeToConsole(level, std::forward<StringType>(message),
			tm, milliseconds,
			shortFile, line, threadName, hasThreadName);
		#endif // Q_OS_WIN

		// 写入文件（如果已初始化）
		if (m_initialized)
		{
			writeToFile(level, std::forward<StringType>(message),
				tm, milliseconds,
				shortFile, line, threadName, hasThreadName);
		}
	}

	// =============== 模式4: RELEASE_MODE ===============
	// 发布模式：只输出到文件
	#elif defined(LOG_RELEASE_MODE)

	template<typename StringType>
		requires StringLiteralOrQString<StringType>
	void Logger::log(LogLevel level, StringType&& message,
		const std::source_location& location)
	{
		std::tm tm;
		int milliseconds;
		const char* file;
		const char* shortFile;
		int line;
		QString threadName;
		bool hasThreadName;

		getLogBasicInfo(location, tm, milliseconds, file, shortFile, line, threadName, hasThreadName);

		// 发布模式：只写入文件
		writeToFile(level, std::forward<StringType>(message),
			tm, milliseconds,
			shortFile, line, threadName, hasThreadName);
	}

	// =============== 模式5: 默认模式 ===============
	// 根据QT_DEBUG和平台自动选择
	#else

	template<typename StringType>
		requires StringLiteralOrQString<StringType>
	void Logger::log(LogLevel level, StringType&& message,
		const std::source_location& location)
	{
		std::tm tm;
		int milliseconds;
		const char* file;
		const char* shortFile;
		int line;
		QString threadName;
		bool hasThreadName;

		getLogBasicInfo(location, tm, milliseconds, file, shortFile, line, threadName, hasThreadName);

		#ifdef QT_DEBUG
		// 调试模式：输出到控制台和文件
		#ifdef Q_OS_WIN
		writeToVSDebug(level, std::forward<StringType>(message),
			tm, milliseconds,
			file, line, threadName, hasThreadName);
		#else
		writeToConsole(level, std::forward<StringType>(message),
			tm, milliseconds,
			shortFile, line, threadName, hasThreadName);
		#endif // Q_OS_WIN

		// 写入文件（如果已初始化）
		if (m_initialized)
		{
			writeToFile(level, std::forward<StringType>(message),
				tm, milliseconds,
				shortFile, line, threadName, hasThreadName);
		}
		#else
		// 发布模式：只写入文件（如果已初始化）
		writeToFile(level, std::forward<StringType>(message),
			tm, milliseconds,
			shortFile, line, threadName, hasThreadName);
		#endif // QT_DEBUG
	}

	#endif // 模式选择

}

// 便捷宏
#define LOG_TRACE(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Trace)) \
	{ \
		logInstance.trace(msg); \
	} \
} while(0)

#define LOG_DEBUG(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Debug)) \
	{ \
		logInstance.debug(msg); \
	} \
} while(0)

#define LOG_INFO(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Info)) \
	{ \
		logInstance.info(msg); \
	} \
} while(0)

#define LOG_WARN(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Warn)) \
	{ \
		logInstance.warn(msg); \
	} \
} while(0)

#define LOG_ERROR(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Error)) \
	{ \
		logInstance.error(msg); \
	} \
} while(0)

#define LOG_FATAL(msg) do \
{ \
	auto& logInstance = nexusdl::log::Logger::instance(); \
	if (logInstance.shouldLog(nexusdl::log::LogLevel::Fatal)) \
	{ \
		logInstance.fatal(msg); \
	} \
} while(0)