// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "logger.h"

// Qt Core
#include <QCoreApplication>
#include <QtConcurrent>

namespace
{
	constexpr int kDefaultFlushIntervalMs{ 60000 };        // 60s
	constexpr qint64 kDefaultMaxFileSize{ 100 * 1024 * 1024 }; // 100MB
	constexpr qint64 kDefaultMaxFiles{ 1000 };
	constexpr qint64 kMaxLogsPerFlush{ 1000 };
	const QString kLogBaseName = QStringLiteral("NexusDL");
}

namespace nexusdl::log {

	Logger& Logger::instance()
	{
		static Logger instance;
		return instance;
	}

	Logger::Logger(QObject* parent)
		: QObject{ parent }
		, m_logLevel{ LogLevel::Info }
		, m_initialized{ false }
		, m_logDir{ QCoreApplication::applicationDirPath() + "/logs" }
		, m_logFile{}
		, m_currentLogPath{}
		, m_currentFileSize{ 0 }
		, m_maxFileSize{ kDefaultMaxFileSize }
		, m_maxFiles{ kDefaultMaxFiles }
		, m_currentBuffer{ nullptr }
		, m_nextBuffer{ nullptr }
		, m_mutex{}
		, m_flushTimer{ QTimer{this} }
		, m_flushInterval{ kDefaultFlushIntervalMs }
		, m_lastFlushTime{ 0 }
		, m_logsSinceLastFlush{ 0 }
	{
		initialize();
	}

	Logger::~Logger()
	{
		// 强制刷新当前缓冲区
		QMutexLocker locker(&m_mutex);
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

		QMutexLocker locker(&m_mutex);

		initialize();

		return true;
	}

	bool Logger::stop()
	{
		if (!m_initialized)
		{
			return false;
		}

		QMutexLocker locker(&m_mutex);

		if (!m_initialized)
		{
			return false;
		}

		writeBufferToFile();
		shutdown();

		return true;
	}

	void Logger::setLogLevel(LogLevel level)
	{
		m_logLevel = level;
	}

	bool Logger::shouldLog(LogLevel level) const
	{
		#ifdef QT_DEBUG
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load());
		#else
		return static_cast<int>(level) >= static_cast<int>(m_logLevel.load()) && m_initialized;
		#endif // QT_DEBUG
	}

	void Logger::setRotation(qint64 maxSize, qint64 maxFiles)
	{
		m_maxFileSize = maxSize;
		m_maxFiles = maxFiles;
	}

	void Logger::setFlushInterval(int milliseconds)
	{
		m_flushInterval = milliseconds;
		m_flushTimer.setInterval(milliseconds);
	}

	void Logger::onFlushTimer()
	{
		// 检查是否需要定时刷新
		QMutexLocker locker(&m_mutex);
		if (!m_initialized)
		{
			return;
		}

		if (!m_currentBuffer->isEmpty())
		{
			// 如果缓冲区有数据，且距离上次写入时间较长，或者有足够多的日志，则刷新
			qint64 now = QDateTime::currentMSecsSinceEpoch();
			bool shouldFlush{ false };

			// 条件1: 距离上次写入超过刷新间隔
			if (now - m_lastFlushTime > m_flushInterval)
			{
				shouldFlush = true;
			}
			// 条件2: 缓冲区使用率超过一定比例
			else if (m_currentBuffer->shouldFlush())
			{
				shouldFlush = true;
			}
			// 条件3: 有足够多的日志条目
			else if (m_logsSinceLastFlush > kMaxLogsPerFlush)
			{
				shouldFlush = true;
			}

			if (shouldFlush)
			{
				writeBufferToFile();
			}
		}
	}

	bool Logger::openLogFile()
	{
		// 确保日志目录存在
		QDir dir(m_logDir);
		if (!dir.exists())
		{
			if (!dir.mkpath("."))
			{
				qCritical() << "Failed to create log directory:" << m_logDir;
				return false;  // 目录创建失败，直接返回
			}
		}

		QString timeStamp = getTimeStamp();
		QString logFileName = QString("%1-%2.log").arg(kLogBaseName).arg(timeStamp);
		m_currentLogPath = QDir(m_logDir).absoluteFilePath(logFileName);

		m_logFile.setFileName(m_currentLogPath);

		if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
		{
			qCritical() << "Failed to open log file:" << m_currentLogPath
				<< "Error:" << m_logFile.errorString();
			return false;
		}

		m_currentFileSize = m_logFile.size();

		return true;
	}

	bool Logger::rotateIfNeeded()
	{
		if (m_currentFileSize >= m_maxFileSize)
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
		QDir logDir(m_logDir);

		// 获取所有日志文件
		QStringList filters;
		filters << QString("%1-*.log").arg(kLogBaseName);

		QStringList logFiles = logDir.entryList(filters, QDir::Files, QDir::Time | QDir::Reversed);

		// 删除超过最大文件数量的旧文件
		while (logFiles.size() > m_maxFiles)
		{
			QString oldestFile = logDir.absoluteFilePath(logFiles.takeFirst());
			QFile::remove(oldestFile);
		}
	}

	bool Logger::writeToBuffer(QByteArray&& message)
	{
		QMutexLocker locker(&m_mutex);
		if (!m_initialized)
		{
			return false;
		}
		// 检查缓冲区是否已满
		if (m_currentBuffer->writableBytes() < message.size())
		{
			// 当前缓冲区满，写入文件
			if (!writeBufferToFile())
			{
				return false;
			}
		}

		// 写入
		m_currentBuffer->append(std::move(message));
		++m_logsSinceLastFlush;
		return true;
	}

	bool Logger::writeBufferToFile()
	{
		// 当前缓冲区满，交换并写入
		std::swap(m_currentBuffer, m_nextBuffer);

		// 获取缓冲区数据的常量引用
		const QByteArray& logData = m_nextBuffer->data();
		qint64 dataSize = logData.size();

		// 如果没有数据，直接返回
		if (dataSize == 0)
		{
			return true;
		}

		qint64 bytesWritten = m_logFile.write(logData);

		// 检查写入是否成功
		if (bytesWritten != dataSize)
		{
			// todo：弹出错误提示
			qWarning() << "QFile write failed, written: " << bytesWritten
				<< " expected: " << dataSize
				<< " error: " << m_logFile.errorString();

			// 暂时：出错情况下清空缓冲区确保两个缓冲区都是空的,如果后续需要添加恢复日志系统的功能则需更改
			m_nextBuffer->clear();
			shutdown();
			return false;
		}

		m_currentFileSize += bytesWritten;

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
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		std::tm tm;
		#ifdef Q_OS_WIN
		localtime_s(&tm, &time);
		#else
		localtime_r(&time, &tm);
		#endif // Q_OS_WIN

		char buffer[64]{};
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", &tm);
		return QString(buffer);
	}

	uint32_t Logger::getCurrentProcessId() const
	{
		return static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	}

	QString Logger::levelToColor(LogLevel level) const
	{
		switch (level)
		{
		case LogLevel::Trace:
			return "\033[36m";
		case LogLevel::Debug:
			return "\033[34m";
		case LogLevel::Info:
			return "\033[32m";
		case LogLevel::Warn:
			return "\033[33m";
		case LogLevel::Error:
			return "\033[31m";
		case LogLevel::Fatal:
			return "\033[35m";
		default:
			return "";
		}
	}

	void Logger::getLogBasicInfo(const std::source_location& location,
		std::tm& tm, int& milliseconds,
		const char*& file, const char*& shortFile,
		int& line, QString& threadName, bool& hasThreadName)
	{
		auto now = std::chrono::system_clock::now();
		auto since_epoch = now.time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(since_epoch);
		milliseconds = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
			since_epoch - seconds).count());

		auto time = std::chrono::system_clock::to_time_t(
			std::chrono::system_clock::time_point(seconds));

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
		const char* lastSlash = std::max(std::strrchr(file, '/'), std::strrchr(file, '\\'));
		if (lastSlash != nullptr)
		{
			shortFile = lastSlash + 1;
		}

		// 获取线程名称
		threadName = QThread::currentThread()->objectName();
		hasThreadName = !threadName.isEmpty();
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
		m_flushTimer.start(m_flushInterval);

		m_initialized = true;

		// 记录初始化信息
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

}