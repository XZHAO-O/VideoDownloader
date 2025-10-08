#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDir>
#include <memory>

class LogSystem : public QObject
{
	Q_OBJECT
public:
	enum LogLevel {
		Trace = 0,
		Debug = 1,
		Info = 2,
		Warning = 3,
		Error = 4,
		Critical = 5
	};

	static LogSystem& instance();

	void initialize(const QString& logDir, LogLevel level = Info);
	void shutdown();

	void log(LogLevel level, const QString& message, const QString& category = "General");
	void setGlobalLevel(LogLevel level);

	// 日志旋转和归档
	void setRotation(size_t maxSize, size_t maxFiles);

	// 便捷日志方法
	void trace(const QString& message, const QString& category = "General");
	void debug(const QString& message, const QString& category = "General");
	void info(const QString& message, const QString& category = "General");
	void warning(const QString& message, const QString& category = "General");
	void error(const QString& message, const QString& category = "General");
	void critical(const QString& message, const QString& category = "General");

signals:
	void logMessage(LogLevel level, const QString& category, const QString& message, const QDateTime& timestamp);

private:
	explicit LogSystem(QObject* parent = nullptr);
	~LogSystem();

	QString levelToString(LogLevel level) const;
	QString formatMessage(LogLevel level, const QString& category, const QString& message) const;
	void rotateIfNeeded();
	void cleanupOldFiles();

	QFile m_logFile;
	QTextStream m_stream;
	QString m_logDir;
	LogLevel m_globalLevel;
	QMutex m_mutex;

	// 日志旋转配置
	size_t m_maxFileSize;
	size_t m_maxFiles;
	bool m_initialized;
};

// 便捷宏
#define LOG_TRACE(category, message)    LogSystem::instance().trace(message, category)
#define LOG_DEBUG(category, message)    LogSystem::instance().debug(message, category)
#define LOG_INFO(category, message)     LogSystem::instance().info(message, category)
#define LOG_WARN(category, message)     LogSystem::instance().warning(message, category)
#define LOG_ERROR(category, message)    LogSystem::instance().error(message, category)
#define LOG_CRITICAL(category, message) LogSystem::instance().critical(message, category)