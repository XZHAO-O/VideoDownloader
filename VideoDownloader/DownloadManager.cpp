#include "DownloadManager.h"
#include "ApplicationController.h"
#include "DownloadOrchestrationService.h"
#include <QStandardPaths>
#include <QDir>

DownloadManager::DownloadManager(QSharedPointer<ApplicationController> appController,
	QObject* parent)
	: QObject(parent)
	, m_appController(appController)
{
	// 设置默认下载路径
	m_defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
		+ "/VideoDownloader";
	QDir dir(m_defaultDownloadPath);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	// 连接编排服务的信号
	if (m_appController && m_appController->getDownloadService()) {
		auto downloadService = m_appController->getDownloadService();
		connect(downloadService.get(), &DownloadOrchestrationService::downloadProgress,
			this, &DownloadManager::onOrchestrationProgress);
		connect(downloadService.get(), &DownloadOrchestrationService::downloadCompleted,
			this, &DownloadManager::onOrchestrationCompleted);
		connect(downloadService.get(), &DownloadOrchestrationService::downloadFailed,
			this, &DownloadManager::onOrchestrationFailed);
	}

	// 添加速度计算定时器
	m_speedTimer = new QTimer(this);
	connect(m_speedTimer, &QTimer::timeout, this, &DownloadManager::calculateAndEmitDownloadSpeed);
	m_speedTimer->start(1000); // 每秒计算一次速度
}

QString DownloadManager::downloadVideo(const VideoDownloadRequest& request)
{
	if (!request.isValid()) {
		LogSystem::instance().error("Invalid download request", "DownloadManager");
		return "";
	}

	QString taskId = request.taskId.isEmpty() ? VideoDownloadRequest::generateTaskId() : request.taskId;

	DownloadTaskInfo taskInfo;
	taskInfo.taskId = taskId;
	taskInfo.request = request;
	taskInfo.status = Queued;
	taskInfo.startTime = QDateTime::currentDateTime();

	// 添加到队列
	m_queuedDownloads[taskId] = taskInfo;
	m_downloadQueue.enqueue(taskId);

	LogSystem::instance().info(QString("Download queued: %1").arg(taskId), "DownloadManager");
	emit downloadAdded(taskId);

	// 处理队列
	processQueue();

	return taskId;
}

// 在 pauseDownload、resumeDownload、cancelDownload 等方法中也添加状态变化信号
void DownloadManager::pauseDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadTaskInfo& info = m_activeDownloads[taskId];
		if (info.status == Downloading) {
			info.status = Paused;
			emit downloadPaused(taskId);
			emit downloadStatusChanged(taskId); // 添加状态变化信号
			LogSystem::instance().info(QString("Download paused: %1").arg(taskId), "DownloadManager");
		}
	}
}

void DownloadManager::resumeDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadTaskInfo& info = m_activeDownloads[taskId];
		if (info.status == Paused) {
			info.status = Downloading;
			emit downloadResumed(taskId);
			emit downloadStatusChanged(taskId); // 添加状态变化信号
			LogSystem::instance().info(QString("Download resumed: %1").arg(taskId), "DownloadManager");
		}
	}
	else if (m_queuedDownloads.contains(taskId)) {
		// 如果任务在队列中，确保它在队列中
		if (!m_downloadQueue.contains(taskId)) {
			m_downloadQueue.enqueue(taskId);
		}
		processQueue();
	}
}

void DownloadManager::cancelDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadTaskInfo info = m_activeDownloads[taskId];
		m_activeDownloads.remove(taskId);
		info.status = Cancelled;
		m_completedDownloads[taskId] = info;

		emit downloadCancelled(taskId);
		emit downloadStatusChanged(taskId); // 添加状态变化信号
		LogSystem::instance().info(QString("Download cancelled: %1").arg(taskId), "DownloadManager");

		m_currentDownloads--;
		processQueue();
	}
	else if (m_queuedDownloads.contains(taskId)) {
		m_queuedDownloads.remove(taskId);
		m_downloadQueue.removeAll(taskId);
		emit downloadCancelled(taskId);
		emit downloadStatusChanged(taskId); // 添加状态变化信号
		LogSystem::instance().info(QString("Queued download cancelled: %1").arg(taskId), "DownloadManager");
	}
}

// 添加下载速度计算方法
void DownloadManager::calculateAndEmitDownloadSpeed()
{
	qint64 totalSpeed = 0;
	QDateTime currentTime = QDateTime::currentDateTime();

	for (auto& task : m_activeDownloads) {
		if (task.status == Downloading) {
			// 计算每个下载任务的速度
			totalSpeed += static_cast<qint64>(task.downloadSpeed);
		}
	}

	emit downloadSpeedUpdated(totalSpeed);
}

// 添加下载速度更新方法
void DownloadManager::updateDownloadSpeed(qint64 bytesPerSecond)
{
	emit downloadSpeedUpdated(bytesPerSecond);
}

QList<QString> DownloadManager::downloadBatch(const QList<VideoDownloadRequest>& requests)
{
	QList<QString> taskIds;
	for (const auto& request : requests) {
		QString taskId = downloadVideo(request);
		if (!taskId.isEmpty()) {
			taskIds.append(taskId);
		}
	}
	return taskIds;
}

void DownloadManager::pauseAll()
{
	for (const QString& taskId : m_activeDownloads.keys()) {
		pauseDownload(taskId);
	}
}

void DownloadManager::resumeAll()
{
	for (const QString& taskId : m_activeDownloads.keys()) {
		resumeDownload(taskId);
	}
}

void DownloadManager::cancelAll()
{
	// 先取消活跃下载
	auto activeTasks = m_activeDownloads.keys();
	for (const QString& taskId : activeTasks) {
		cancelDownload(taskId);
	}

	// 然后取消队列中的下载
	auto queuedTasks = m_queuedDownloads.keys();
	for (const QString& taskId : queuedTasks) {
		cancelDownload(taskId);
	}
}

QList<DownloadTaskInfo> DownloadManager::getActiveDownloads() const
{
	return m_activeDownloads.values();
}

QList<DownloadTaskInfo> DownloadManager::getCompletedDownloads() const
{
	return m_completedDownloads.values();
}

QList<DownloadTaskInfo> DownloadManager::getQueuedDownloads() const
{
	return m_queuedDownloads.values();
}

DownloadTaskInfo DownloadManager::getDownloadInfo(const QString& taskId) const
{
	if (m_activeDownloads.contains(taskId)) {
		return m_activeDownloads.value(taskId);
	}
	else if (m_queuedDownloads.contains(taskId)) {
		return m_queuedDownloads.value(taskId);
	}
	else if (m_completedDownloads.contains(taskId)) {
		return m_completedDownloads.value(taskId);
	}
	return DownloadTaskInfo();
}

void DownloadManager::setMaxConcurrentDownloads(int count)
{
	m_maxConcurrentDownloads = qMax(1, count);
	processQueue();
}

int DownloadManager::getMaxConcurrentDownloads() const
{
	return m_maxConcurrentDownloads;
}

void DownloadManager::setDefaultDownloadPath(const QString& path)
{
	m_defaultDownloadPath = path;
	QDir dir(path);
	if (!dir.exists()) {
		dir.mkpath(".");
	}
}

QString DownloadManager::getDefaultDownloadPath() const
{
	return m_defaultDownloadPath;
}

void DownloadManager::onOrchestrationProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadTaskInfo& info = m_activeDownloads[taskId];
		info.updateProgress(downloaded, total);
		emit downloadProgress(taskId, downloaded, total);
		emit downloadStatusChanged(taskId); // 添加状态变化信号
	}
}

void DownloadManager::onOrchestrationCompleted(const QString& taskId, const QString& filePath)
{
	completeDownload(taskId, true, filePath);
	emit downloadCompleted(taskId, filePath);
	emit downloadStatusChanged(taskId); // 添加状态变化信号
}

void DownloadManager::onOrchestrationFailed(const QString& taskId, const QString& error)
{
	completeDownload(taskId, false);
	emit downloadFailed(taskId, error);
	emit downloadStatusChanged(taskId); // 添加状态变化信号
}

void DownloadManager::processQueue()
{
	while (m_currentDownloads < m_maxConcurrentDownloads && !m_downloadQueue.isEmpty()) {
		startNextDownload();
	}
}

void DownloadManager::startNextDownload()
{
	if (m_downloadQueue.isEmpty()) {
		return;
	}

	QString taskId = m_downloadQueue.dequeue();
	if (!m_queuedDownloads.contains(taskId)) {
		return;
	}

	DownloadTaskInfo taskInfo = m_queuedDownloads.value(taskId);
	m_queuedDownloads.remove(taskId);

	taskInfo.status = Downloading;
	m_activeDownloads[taskId] = taskInfo;
	m_currentDownloads++;

	// 这里应该调用编排服务开始下载
	if (auto downloadService = m_appController->getDownloadService()) {
		downloadService->startDownload(taskInfo.request);
	}

	LogSystem::instance().info(QString("Download started: %1").arg(taskId), "DownloadManager");
}

void DownloadManager::updateDownloadInfo(const QString& taskId, const DownloadTaskInfo& info)
{
	if (m_activeDownloads.contains(taskId)) {
		m_activeDownloads[taskId] = info;
	}
}

void DownloadManager::completeDownload(const QString& taskId, bool success, const QString& filePath)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadTaskInfo info = m_activeDownloads.value(taskId);
		m_activeDownloads.remove(taskId);

		info.status = success ? Completed : Failed;
		info.endTime = QDateTime::currentDateTime();
		if (!filePath.isEmpty()) {
			// 这里可以设置文件路径等信息
		}

		m_completedDownloads[taskId] = info;
		m_currentDownloads--;

		LogSystem::instance().info(
			QString("Download %1: %2").arg(success ? "completed" : "failed").arg(taskId),
			"DownloadManager");

		processQueue();
	}
}