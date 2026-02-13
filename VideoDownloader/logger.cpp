// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "logger.h"

// Qt Core
#include <QCoreApplication>
#include <QtConcurrent>

namespace
{
	constexpr int kFlushInterval{ 60000 };        // 60s
	constexpr qint64 kDefaultMaxFileSize{ 100 * 1024 * 1024 }; // 100MB
	constexpr qint64 kDefaultMaxFiles{ 1000 };
	constexpr qint64 kMaxLogsPerFlush{ 1000 };
	const QString kLogBaseName{ QStringLiteral("NexusDL") };
}

namespace nexusdl::log {

	Logger& Logger::instance()
	{
		static Logger instance{};
		return instance;
	}

	Logger::Logger(QObject* parent)
		: QObject{ parent }
		, m_logLevel{ LogLevel::Info }
		, m_initialized{ false }
		, m_logDir{ QCoreApplication::applicationDirPath() % "/logs" }
		, m_logFile{}
		, m_currentLogPath{}
		, m_currentFileSize{ 0 }
		, m_maxFileSize{ kDefaultMaxFileSize }
		, m_maxFileCount{ kDefaultMaxFiles }
		, m_currentBuffer{ nullptr }
		, m_nextBuffer{ nullptr }
		, m_mutex{}
		, m_flushTimer{ QTimer{this} }
		, m_lastFlushTime{ 0 }
		, m_logsSinceLastFlush{ 0 }
	{
		initialize();
	}

	Logger::~Logger()
	{
		// 强制刷新当前缓冲区
		QMutexLocker locker{ &m_mutex };
		if (!m_initialized)
		{
			return;
		}

		writeBufferToFile();
		shutdown();
	}

	bool Logger::start()
	{
		if (m_initialized)
		{
			return false;
		}

		QMutexLocker locker{ &m_mutex };

		initialize();

		return true;
	}

	bool Logger::stop()
	{
		if (!m_initialized)
		{
			return false;
		}

		QMutexLocker locker{ &m_mutex };

		if (!m_initialized)
		{
			return false;
		}

		writeBufferToFile();
		shutdown();

		return true;
	}

	void Logger::setLogLevel(LogLevel level) noexcept
	{
		m_logLevel.store(level, std::memory_order_relaxed);
	}

	// =============== shouldLog 函数模式 ===============
	#ifdef LOG_VSOUTPUT_MODE
	// 模式1: VSOUTPUT_MODE - 仅输出到VS调试控制台，不需要检查初始化
	bool Logger::shouldLog(LogLevel level) const noexcept
	{
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed));
	}
	#elif defined(LOG_CONSOLEOUTPUT_MODE)
	// 模式2: CONSOLEOUTPUT_MODE - 仅输出到彩色控制台，不需要检查初始化
	bool Logger::shouldLog(LogLevel level) const noexcept
	{
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed));
	}
	#elif defined(LOG_DEBUG_MODE)
	// 模式3: DEBUG_MODE - 同时输出到控制台和文件
	bool Logger::shouldLog(LogLevel level) const noexcept
	{
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed));
	}
	#elif defined(LOG_RELEASE_MODE)
	// 模式4: RELEASE_MODE - 只输出到文件，需要检查初始化
	bool Logger::shouldLog(LogLevel level) const noexcept
	{
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed)) && m_initialized;
	}
	#else
	// 默认模式 - 根据QT_DEBUG决定
	bool Logger::shouldLog(LogLevel level) const noexcept
	{
		#ifdef QT_DEBUG
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed));
		#else
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load(std::memory_order_relaxed)) && m_initialized;
		#endif // QT_DEBUG
	}
	#endif // 模式选择

	void Logger::setMaxRotationFileSize(qint64 maxFileSize) noexcept
	{
		m_maxFileSize.store(maxFileSize, std::memory_order_relaxed);
	}

	void Logger::setMaxRotationFileCount(qint64 maxFileCount) noexcept
	{
		m_maxFileCount.store(maxFileCount, std::memory_order_relaxed);
	}

	void Logger::onFlushTimer()
	{
		// 检查是否需要定时刷新
		QMutexLocker locker{ &m_mutex };
		if (!m_initialized)
		{
			return;
		}

		if (!m_currentBuffer->isEmpty())
		{
			// 如果缓冲区有数据，且距离上次写入时间较长，或者有足够多的日志，则刷新
			if (QDateTime::currentMSecsSinceEpoch() - m_lastFlushTime > kFlushInterval || m_currentBuffer->shouldFlush() || m_logsSinceLastFlush > kMaxLogsPerFlush)
			{
				writeBufferToFile();
			}
		}
	}

	bool Logger::openLogFile()
	{
		// 确保日志目录存在
		QDir dir{ m_logDir };
		if (!dir.exists())
		{
			if (!dir.mkpath("."))
			{
				qCritical() << "Failed to create log directory:" << m_logDir;
				return false;  // 目录创建失败，直接返回
			}
		}

		m_currentLogPath = dir.absoluteFilePath(kLogBaseName % "-" % getTimeStamp() % ".log");

		m_logFile.setFileName(m_currentLogPath);

		if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		{
			qCritical() << "Failed to open log file:" << m_currentLogPath << "Error:" << m_logFile.errorString();
			return false;
		}

		m_currentFileSize = m_logFile.size();

		return true;
	}

	bool Logger::rotateIfNeeded()
	{
		if (m_currentFileSize >= m_maxFileSize.load(std::memory_order_relaxed))
		{
			// 关闭当前文件
			m_logFile.flush();
			m_logFile.close();

			// 创建新文件
			if (!openLogFile())
			{
				shutdown();
				return false;
			}

			// 清理旧文件
			cleanupOldFiles();
		}
		return true;
	}

	void Logger::cleanupOldFiles()
	{
		QDir logDir{ m_logDir };

		// 获取所有日志文件
		QStringList logFiles = logDir.entryList({ kLogBaseName % "-*.log" }, QDir::Files, QDir::Time | QDir::Reversed);

		// 删除超过最大文件数量的旧文件
		while (logFiles.size() > m_maxFileCount.load(std::memory_order_relaxed))
		{
			QFile::remove(logDir.absoluteFilePath(logFiles.takeFirst()));
		}
	}

	bool Logger::writeToBuffer(const char* message, qint64 size)
	{
		QMutexLocker locker{ &m_mutex };
		if (!m_initialized)
		{
			return false;
		}
		// 检查缓冲区是否已满
		if (m_currentBuffer->writableBytes() < size)
		{
			// 当前缓冲区满，写入文件
			if (!writeBufferToFile())
			{
				return false;
			}
		}

		// 写入
		m_currentBuffer->append(message, size);
		++m_logsSinceLastFlush;
		return true;
	}

	bool Logger::writeBufferToFile()
	{
		// 当前缓冲区满，交换并写入
		std::swap(m_currentBuffer, m_nextBuffer);

		// 获取缓冲区数据的常量引用
		const QByteArray& logData = m_nextBuffer->data();

		// 如果没有数据，直接返回
		if (const qint64 dataSize = logData.size(); dataSize == 0)
		{
			return true;
		}
		else
		{
			// 检查写入是否成功
			if (const qint64 bytesWritten = m_logFile.write(logData); bytesWritten != dataSize)
			{
				// todo：弹出错误提示
				qWarning() << "QFile write failed, written: " << bytesWritten << " expected: " << dataSize << " error: " << m_logFile.errorString();

				shutdown();
				return false;
			}
			else
			{
				m_currentFileSize += bytesWritten;
			}
		}

		// 刷新文件流到磁盘
		m_logFile.flush();

		m_lastFlushTime = QDateTime::currentMSecsSinceEpoch();

		// 清空缓冲区
		m_nextBuffer->clear();

		//确保成功写入才清空待写入日志数
		m_logsSinceLastFlush = 0;

		// 检查是否需要日志轮转（放在后面以确保即使日志轮转发生错误时也不影响将错误前最后的日志写入）
		if (!rotateIfNeeded())
		{
			// todo：弹出错误提示
			return false;
		}

		return true;
	}

	QString Logger::getTimeStamp() const
	{
		thread_local std::array<char, 20> buffer;

		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		#ifdef Q_OS_WIN
		localtime_s(&tm, &time);
		#else
		localtime_r(&time, &tm);
		#endif // Q_OS_WIN

		std::strftime(buffer.data(), buffer.size(), "%Y-%m-%d-%H-%M-%S", &tm);
		return QString::fromLatin1(buffer.data());
	}

	uint32_t Logger::getCurrentThreadId() const
	{
		return static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	}

	void Logger::getLogBasicInfo(const std::source_location& location,
		std::tm& tm, int& milliseconds,
		const char*& file, const char*& shortFile,
		int& line, QString& threadName, bool& hasThreadName)
	{
		auto now = std::chrono::system_clock::now();
		auto since_epoch = now.time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
		milliseconds = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch - seconds).count());

		auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(seconds));

		#ifdef Q_OS_WIN
		localtime_s(&tm, &time);
		#else
		localtime_r(&time, &tm);
		#endif

		// 获取文件名和行号
		file = location.file_name();
		line = location.line();

		// 获取短文件名
		shortFile = file;

		if (const char* lastSlash = std::max(std::strrchr(file, '/'), std::strrchr(file, '\\')); lastSlash != nullptr)
		{
			shortFile = lastSlash + 1;
		}

		// 获取线程名称
		threadName = QThread::currentThread()->objectName();
		hasThreadName = !threadName.isEmpty();
	}

	char* Logger::writeString(char* ptr, const char* str)
	{
		const size_t len = std::strlen(str);
		memcpy(ptr, str, len);
		return ptr + len;
	}

	char* Logger::writeTwoDigits(char* ptr, int value)
	{
		if (value < 10)
		{
			*ptr++ = '0';
		}
		return writeNumber(ptr, value);
	}

	char* Logger::writeThreeDigits(char* ptr, int value)
	{
		if (value < 100) *ptr++ = '0';
		if (value < 10) *ptr++ = '0';
		return writeNumber(ptr, value);
	}

	void Logger::initialize()
	{
		// 打开日志文件
		if (!openLogFile())
		{
			// 错误处理待添加
			m_initialized = false;
			return;
		}

		// 只有文件打开成功后才创建缓冲区和定时器
		if (!m_currentBuffer)
			m_currentBuffer = std::make_unique<LogBuffer>();
		if (!m_nextBuffer)
			m_nextBuffer = std::make_unique<LogBuffer>();

		// 创建定时器
		connect(&m_flushTimer, &QTimer::timeout, this, &Logger::onFlushTimer);
		m_flushTimer.start(kFlushInterval);

		m_initialized = true;

		// 记录初始化信息
		if (shouldLog(LogLevel::Info))
			info("LogSystem start successfully");
	}

	void Logger::shutdown()
	{
		// 停止定时器
		disconnect(&m_flushTimer, nullptr, nullptr, nullptr);
		if (m_flushTimer.isActive())
			m_flushTimer.stop();

		// 刷新并关闭文件
		if (m_logFile.isOpen())
		{
			m_logFile.flush();
			m_logFile.close();
		}

		m_initialized = false;
	}

} // namespace nexusdl::log