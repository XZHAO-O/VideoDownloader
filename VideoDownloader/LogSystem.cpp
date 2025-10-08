#include "LogSystem.h"
#include <QCoreApplication>

LogSystem& LogSystem::instance()
{
	static LogSystem instance;
	return instance;
}

LogSystem::LogSystem(QObject* parent)
	: QObject(parent)
	, m_globalLevel(Info)
	, m_maxFileSize(10 * 1024 * 1024) // 10MB
	, m_maxFiles(5)
	, m_initialized(false)
{
}

LogSystem::~LogSystem()
{
	shutdown();
}

void LogSystem::initialize(const QString& logDir, LogLevel level)
{
	QMutexLocker locker(&m_mutex);

	if (m_initialized) {
		return;
	}

	m_logDir = logDir;
	m_globalLevel = level;

	// 确保日志目录存在
	QDir dir(logDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	// 创建日志文件
	QString logFilePath = dir.absoluteFilePath("application.log");
	m_logFile.setFileName(logFilePath);

	if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
		qWarning() << "Failed to open log file:" << logFilePath;
		return;
	}

	m_stream.setDevice(&m_logFile);
	m_initialized = true;

	// 记录初始化信息
	info("Log system initialized", "LogSystem");
}

void LogSystem::shutdown()
{
	QMutexLocker locker(&m_mutex);

	if (m_initialized) {
		info("Log system shutting down", "LogSystem");
		m_stream.flush();
		m_logFile.close();
		m_initialized = false;
	}
}

void LogSystem::setGlobalLevel(LogLevel level)
{
	QMutexLocker locker(&m_mutex);
	m_globalLevel = level;
}

void LogSystem::setRotation(size_t maxSize, size_t maxFiles)
{
	QMutexLocker locker(&m_mutex);
	m_maxFileSize = maxSize;
	m_maxFiles = maxFiles;
}

void LogSystem::log(LogLevel level, const QString& message, const QString& category)
{
	if (level < m_globalLevel || !m_initialized) {
		return;
	}

	QMutexLocker locker(&m_mutex);

	// 检查是否需要日志轮转
	rotateIfNeeded();

	QString formattedMessage = formatMessage(level, category, message);

	// 输出到文件
	m_stream << formattedMessage << "\n";
	m_stream.flush();

	// 同时输出到控制台（在调试模式下）
	#ifdef QT_DEBUG
	QTextStream console(stdout);
	console << formattedMessage << "\n";
	#endif

	// 发出信号
	emit logMessage(level, category, message, QDateTime::currentDateTime());
}

void LogSystem::trace(const QString& message, const QString& category)
{
	log(Trace, message, category);
}

void LogSystem::debug(const QString& message, const QString& category)
{
	log(Debug, message, category);
}

void LogSystem::info(const QString& message, const QString& category)
{
	log(Info, message, category);
}

void LogSystem::warning(const QString& message, const QString& category)
{
	log(Warning, message, category);
}

void LogSystem::error(const QString& message, const QString& category)
{
	log(Error, message, category);
}

void LogSystem::critical(const QString& message, const QString& category)
{
	log(Critical, message, category);
}

QString LogSystem::levelToString(LogLevel level) const
{
	switch (level) {
	case Trace: return "TRACE";
	case Debug: return "DEBUG";
	case Info: return "INFO";
	case Warning: return "WARN";
	case Error: return "ERROR";
	case Critical: return "CRITICAL";
	default: return "UNKNOWN";
	}
}

QString LogSystem::formatMessage(LogLevel level, const QString& category, const QString& message) const
{
	QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
	QString levelStr = levelToString(level);

	return QString("[%1] [%2] [%3] %4")
		.arg(timestamp)
		.arg(levelStr, -7)
		.arg(category, -15)
		.arg(message);
}

void LogSystem::rotateIfNeeded()
{
	if (m_logFile.size() >= m_maxFileSize) {
		m_stream.flush();
		m_logFile.close();

		QString currentPath = m_logFile.fileName();
		QFileInfo fileInfo(currentPath);

		// 重命名现有文件
		for (int i = m_maxFiles - 1; i > 0; --i) {
			QString oldName = fileInfo.absolutePath() + "/" + fileInfo.baseName() + "." + QString::number(i) + "." + fileInfo.completeSuffix();
			QString newName = fileInfo.absolutePath() + "/" + fileInfo.baseName() + "." + QString::number(i + 1) + "." + fileInfo.completeSuffix();

			if (QFile::exists(oldName)) {
				QFile::rename(oldName, newName);
			}
		}

		// 将当前文件重命名为 .1
		QString firstBackup = fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".1." + fileInfo.completeSuffix();
		QFile::rename(currentPath, firstBackup);

		// 重新打开日志文件
		if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
			m_stream.setDevice(&m_logFile);
		}

		// 清理过期的日志文件
		cleanupOldFiles();
	}
}

void LogSystem::cleanupOldFiles()
{
	QFileInfo currentFileInfo(m_logFile.fileName());
	QString pattern = currentFileInfo.absolutePath() + "/" + currentFileInfo.baseName() + ".*." + currentFileInfo.completeSuffix();

	QDir logDir(currentFileInfo.absolutePath());
	QStringList logFiles = logDir.entryList(QStringList() << pattern, QDir::Files, QDir::Name);

	// 删除超过最大文件数量的旧日志
	while (logFiles.size() > m_maxFiles) {
		QString oldestFile = logDir.absoluteFilePath(logFiles.takeFirst());
		QFile::remove(oldestFile);
	}
}