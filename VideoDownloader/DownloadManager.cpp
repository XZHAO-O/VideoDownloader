#include "DownloadManager.h"
#include "DownloadEngine.h"
#include <QUuid>

DownloadManager::DownloadManager(QObject* parent)
	: QObject(parent)
	, m_engine(new DownloadEngine())
{
	// 将引擎移动到工作线程
	m_engine->moveToThread(&m_workerThread);

	// 连接从引擎到管理器的信号（跨线程连接，使用QueuedConnection）
	connect(m_engine, &DownloadEngine::downloadPaused,
		this, &DownloadManager::onDownloadPaused, Qt::QueuedConnection);
	connect(m_engine, &DownloadEngine::downloadResumed,
		this, &DownloadManager::onDownloadResumed, Qt::QueuedConnection);
	connect(m_engine, &DownloadEngine::downloadCanceled,
		this, &DownloadManager::onDownloadCanceled, Qt::QueuedConnection);
	connect(m_engine, &DownloadEngine::downloadCompleted,
		this, &DownloadManager::onDownloadCompleted, Qt::QueuedConnection);
	connect(m_engine, &DownloadEngine::downloadFailed,
		this, &DownloadManager::onDownloadFailed, Qt::QueuedConnection);
	connect(m_engine, &DownloadEngine::downloadProgress,
		this, &DownloadManager::onDownloadProgress, Qt::QueuedConnection);

	// 连接从管理器到引擎的信号
	connect(this, &DownloadManager::addDownloadRequested,
		m_engine, &DownloadEngine::onAddDownload, Qt::QueuedConnection);
	connect(this, &DownloadManager::pauseDownloadRequested,
		m_engine, &DownloadEngine::onPauseDownload, Qt::QueuedConnection);
	connect(this, &DownloadManager::resumeDownloadRequested,
		m_engine, &DownloadEngine::onResumeDownload, Qt::QueuedConnection);
	connect(this, &DownloadManager::cancelDownloadRequested,
		m_engine, &DownloadEngine::onCancelDownload, Qt::QueuedConnection);
	connect(this, &DownloadManager::speedLimitChanged,
		m_engine, &DownloadEngine::onSpeedLimitChanged, Qt::QueuedConnection);
	connect(this, &DownloadManager::maxConcurrentChanged,
		m_engine, &DownloadEngine::onMaxConcurrentChanged, Qt::QueuedConnection);
	connect(this, &DownloadManager::maxThreadsChanged,
		m_engine, &DownloadEngine::onMaxThreadsChanged, Qt::QueuedConnection);

	// 启动工作线程
	m_workerThread.start();
}

DownloadManager::~DownloadManager()
{
	m_workerThread.quit();
	m_workerThread.wait();
	delete m_engine;
}

void DownloadManager::addDownload(const DownloadTaskInfo& taskInfo)
{
	// 确保任务状态正确
	DownloadTaskInfo updatedTaskInfo = taskInfo;
	if (updatedTaskInfo.status != Queued && updatedTaskInfo.status != Downloading) {
		updatedTaskInfo.status = Queued;
	}

	// 设置开始时间
	updatedTaskInfo.startTime = QDateTime::currentDateTime();

	m_tasks[updatedTaskInfo.taskId] = updatedTaskInfo;

	emit addDownloadRequested(updatedTaskInfo);
	emit downloadAdded(updatedTaskInfo.taskId);
}

void DownloadManager::pauseDownload(const QString& taskId)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Paused;
		emit pauseDownloadRequested(taskId);
		emit downloadPaused(taskId);
	}
}

void DownloadManager::resumeDownload(const QString& taskId)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Downloading;
		emit resumeDownloadRequested(taskId);
		emit downloadResumed(taskId);
	}
}

void DownloadManager::cancelDownload(const QString& taskId)
{
	if (m_tasks.contains(taskId)) {
		m_tasks.remove(taskId);
		emit cancelDownloadRequested(taskId);
		emit downloadCanceled(taskId);
	}
}

void DownloadManager::setMaxConcurrentDownloads(int max)
{
	emit maxConcurrentChanged(max);
}

void DownloadManager::setDownloadSpeedLimit(qint64 bytesPerSecond)
{
	emit speedLimitChanged(bytesPerSecond);
}

void DownloadManager::setMaxThreadsPerDownload(int maxThreads)
{
	emit maxThreadsChanged(maxThreads);
}

DownloadTaskInfo DownloadManager::getTaskInfo(const QString& taskId) const
{
	return m_tasks.value(taskId);
}

QList<DownloadTaskInfo> DownloadManager::getAllTasks() const
{
	return m_tasks.values();
}

QString DownloadManager::generateTaskId() const
{
	return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void DownloadManager::onDownloadPaused(const QString& taskId)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Paused;
	}
	emit downloadPaused(taskId);
}

void DownloadManager::onDownloadResumed(const QString& taskId)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Downloading;
	}
	emit downloadResumed(taskId);
}

void DownloadManager::onDownloadCanceled(const QString& taskId)
{
	m_tasks.remove(taskId);
	emit downloadCanceled(taskId);
}

void DownloadManager::onDownloadCompleted(const QString& taskId, const QString& filePath)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Completed;
		m_tasks[taskId].endTime = QDateTime::currentDateTime();
	}
	emit downloadCompleted(taskId, filePath);
}

void DownloadManager::onDownloadFailed(const QString& taskId, const QString& error)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].status = Failed;
		m_tasks[taskId].errorMessage = error;
		m_tasks[taskId].endTime = QDateTime::currentDateTime();
	}
	emit downloadFailed(taskId, error);
}

void DownloadManager::onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	if (m_tasks.contains(taskId)) {
		m_tasks[taskId].updateProgress(downloaded, total);
	}
	emit downloadProgress(taskId, downloaded, total);
}