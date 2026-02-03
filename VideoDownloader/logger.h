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

// Project internal headers
#include "log_level.h"
#include "log_buffer.h"

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

		// 核心日志方法
		template<typename StringType>
			requires StringLiteralOrQString<StringType>
		void log(LogLevel level, StringType&& message,
			const std::source_location& location = std::source_location::current())
		{
			#ifdef QT_DEBUG
			// 调试模式：同时输出到控制台和文件

			// 获取时间和基本信息
			auto now = std::chrono::system_clock::now();
			auto since_epoch = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				since_epoch - seconds);

			auto time = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::time_point(seconds));

			std::tm tm;
			#ifdef Q_OS_WIN
			localtime_s(&tm, &time);
			#else
			localtime_r(&time, &tm);
			#endif // Q_OS_WIN

			// 获取文件名和行号
			const char* file = location.file_name();
			int line = location.line();

			// 获取短文件名
			const char* shortFile = file;
			const char* lastSlash = std::max(std::strrchr(file, '/'), std::strrchr(file, '\\'));
			if (lastSlash != nullptr)
			{
				shortFile = lastSlash + 1;
			}

			// 如果系统已初始化，则写入文件
			if (m_initialized)
			{
				// thread_local 缓冲区
				static thread_local QByteArray logData;
				logData.clear();  // 清除之前的内容

				if constexpr (std::is_same_v<std::decay_t<StringType>, QString>)
				{
					// QString版本 - 保持原来的多次追加方式
					static thread_local char basePart[256];

					auto [end, ec] = std::format_to_n(
						basePart, sizeof(basePart) - 1,
						"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): ",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
						tm.tm_hour, tm.tm_min, tm.tm_sec,
						static_cast<int>(ms.count()),
						levelToString(level),
						getCurrentProcessId(),
						shortFile,
						line
					);

					*end = '\0';

					// 预分配空间（使用先前的大小或默认值）
					if (logData.capacity() < 1024)
					{
						logData.reserve(1024);
					}

					logData.append(basePart, end - basePart);
					logData.append(std::forward<StringType>(message).toUtf8());
					logData.append('\n');
				}
				else
				{
					// 字符串字面量版本 - 一次性写入到QByteArray的缓冲区
					const char* messagePtr = std::forward<StringType>(message);

					// 使用std::formatted_size计算确切长度
					size_t totalLength = std::formatted_size(
						"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): {}\n",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
						tm.tm_hour, tm.tm_min, tm.tm_sec,
						static_cast<int>(ms.count()),
						levelToString(level),
						getCurrentProcessId(),
						shortFile,
						line,
						messagePtr
					);

					if (logData.capacity() < static_cast<int>(totalLength + 1))
					{
						logData.reserve(totalLength + 1);
					}

					// 直接格式化到QByteArray的缓冲区
					char* dataPtr = logData.data(); // 获取可写指针
					auto [end, ec] = std::format_to_n(
						dataPtr, totalLength + 1, // 使用计算的长度+1作为缓冲区大小
						"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): {}\n",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
						tm.tm_hour, tm.tm_min, tm.tm_sec,
						static_cast<int>(ms.count()),
						levelToString(level),
						getCurrentProcessId(),
						shortFile,
						line,
						messagePtr
					);

					// 设置QByteArray的实际大小
					logData.resize(end - dataPtr);
				}

				// 写入缓冲区
				writeToBuffer(std::move(logData));
			}

			// 使用qDebug输出到控制台（调试模式）
			QString consoleOutput;

			char timeBuffer[68];
			size_t len = std::strftime(timeBuffer, sizeof(timeBuffer) - 4,
				"%Y-%m-%d %H:%M:%S", &tm);
			std::snprintf(timeBuffer + len, 5, ".%03d", static_cast<int>(ms.count()));
			QString debugBasePart = QString("[%1] [%2] [%3]")
				.arg(timeBuffer)
				.arg(levelToString(level))
				.arg(getCurrentProcessId());

			#ifdef Q_OS_WIN
			// Windows环境下：使用VS格式，方便双击跳转
			consoleOutput = QString("%1\n%2(%3): %4\n")
				.arg(debugBasePart)
				.arg(QDir::toNativeSeparators(QString::fromUtf8(file)))
				.arg(line)
				.arg(std::forward<StringType>(message));//对于右值，文件部分只移动了通过toutf8临时构造的qbytearray
			#else
			// 非Windows平台：带颜色的控制台输出
			QString color = levelToColor(level);
			QString resetColor = "\033[0m";

			consoleOutput = QString("%1%2 %3(%4): %5%6")
				.arg(color)
				.arg(debugBasePart)
				.arg(shortFile)
				.arg(line)
				.arg(std::forward<StringType>(message))
				.arg(resetColor);
			#endif // Q_OS_WIN

			qDebug().noquote() << consoleOutput;

			#else
			// 发布模式：只写入文件

			// 获取时间和基本信息
			auto now = std::chrono::system_clock::now();
			auto since_epoch = now.time_since_epoch();
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				since_epoch - seconds);

			auto time = std::chrono::system_clock::to_time_t(
				std::chrono::system_clock::time_point(seconds));

			std::tm tm;
			#ifdef Q_OS_WIN
			localtime_s(&tm, &time);
			#else
			localtime_r(&time, &tm);
			#endif // Q_OS_WIN

			// 获取文件名和行号
			const char* file = location.file_name();
			int line = location.line();

			// 获取短文件名
			const char* shortFile = file;
			const char* lastSlash = std::max(std::strrchr(file, '/'), std::strrchr(file, '\\'));
			if (lastSlash != nullptr)
			{
				shortFile = lastSlash + 1;
			}

			// thread_local 缓冲区
			static thread_local QByteArray logData;
			logData.clear();  // 清除之前的内容

			if constexpr (std::is_same_v<std::decay_t<StringType>, QString>)
			{
				// QString版本 - 保持原来的多次追加方式
				static thread_local char basePart[256];

				//静态部分最长42，行号不超过100w，6位，
				auto [end, ec] = std::format_to_n(
					basePart, sizeof(basePart) - 1,
					"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): ",
					tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					static_cast<int>(ms.count()),
					levelToString(level),
					getCurrentProcessId(),
					shortFile,
					line
				);

				*end = '\0';

				// 预分配空间（使用先前的大小或默认值）
				if (logData.capacity() < 1024)
				{
					logData.reserve(1024);
				}

				logData.append(basePart, end - basePart);
				logData.append(std::forward<StringType>(message).toUtf8());
				logData.append('\n');
			}
			else
			{
				// 字符串字面量版本 - 一次性写入到QByteArray的缓冲区
				const char* messagePtr = std::forward<StringType>(message);

				// 使用std::formatted_size计算确切长度
				size_t totalLength = std::formatted_size(
					"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): {}\n",
					tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					static_cast<int>(ms.count()),
					levelToString(level),
					getCurrentProcessId(),
					shortFile,
					line,
					messagePtr
				);

				if (logData.capacity() < static_cast<int>(totalLength + 1))
				{
					logData.reserve(totalLength + 1);
				}

				// 直接格式化到QByteArray的缓冲区
				char* dataPtr = logData.data(); // 获取可写指针
				auto [end, ec] = std::format_to_n(
					dataPtr, totalLength + 1, // 使用计算的长度+1作为缓冲区大小
					"[{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}] [{}] [{}] {}({}): {}\n",
					tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec,
					static_cast<int>(ms.count()),
					levelToString(level),
					getCurrentProcessId(),
					shortFile,
					line,
					messagePtr
				);

				// 设置QByteArray的实际大小
				logData.resize(end - dataPtr);
			}

			// 写入缓冲区
			writeToBuffer(std::move(logData));
			#endif // QT_DEBUG
		}

		// 便捷日志方法 - 模板化以支持完美转发
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