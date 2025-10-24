#include "DownloadEngine.h"
#include <QDir>
#include <QFileInfo>

DownloadEngine::DownloadEngine(QObject* parent)
	: QObject(parent)
	, m_networkManager(new QNetworkAccessManager(this))
	, m_maxConcurrentDownloads(3)
	, m_currentDownloads(0)
	, m_downloadSpeedLimit(0)
	, m_maxThreadsPerDownload(1)
{
}

DownloadEngine::~DownloadEngine()
{
	// 清理所有活动下载
	for (auto item : m_activeDownloads) {
		item->cancel();
		delete item;
	}
	m_activeDownloads.clear();
}

void DownloadEngine::onAddDownload(const DownloadTaskInfo& taskInfo)
{
	QMutexLocker locker(&m_queueMutex);
	m_downloadQueue.enqueue(taskInfo);
	m_allTasks[taskInfo.taskId] = taskInfo;

	emit downloadAdded(taskInfo.taskId);

	// 立即尝试处理队列
	processQueue();
}

void DownloadEngine::onPauseDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		m_activeDownloads[taskId]->pause();
		emit downloadPaused(taskId);
	}
}

void DownloadEngine::onResumeDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		m_activeDownloads[taskId]->resume();
		emit downloadResumed(taskId);
	}
	else if (m_allTasks.contains(taskId)) {
		// 重新加入队列
		QMutexLocker locker(&m_queueMutex);
		m_downloadQueue.enqueue(m_allTasks[taskId]);
		emit downloadResumed(taskId);
		processQueue();
	}
}

void DownloadEngine::onCancelDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		m_activeDownloads[taskId]->cancel();
		cleanupDownload(taskId);
		emit downloadCanceled(taskId);
	}
	else {
		// 从队列中移除
		QMutexLocker locker(&m_queueMutex);
		for (int i = 0; i < m_downloadQueue.size(); ++i) {
			if (m_downloadQueue[i].taskId == taskId) {
				m_downloadQueue.removeAt(i);
				break;
			}
		}
		m_allTasks.remove(taskId);
		emit downloadCanceled(taskId);
	}
}

void DownloadEngine::onSpeedLimitChanged(qint64 bytesPerSecond)
{
	m_downloadSpeedLimit = bytesPerSecond;
}

void DownloadEngine::onMaxConcurrentChanged(int max)
{
	m_maxConcurrentDownloads = max;
	// 配置改变后立即处理队列
	processQueue();
}

void DownloadEngine::onMaxThreadsChanged(int maxThreads)
{
	m_maxThreadsPerDownload = maxThreads;
}

void DownloadEngine::processQueue()
{
	while (m_currentDownloads < m_maxConcurrentDownloads && !m_downloadQueue.isEmpty()) {
		QMutexLocker locker(&m_queueMutex);
		if (m_downloadQueue.isEmpty()) break;

		DownloadTaskInfo taskInfo = m_downloadQueue.dequeue();
		locker.unlock();

		startNextDownload(taskInfo);
	}
}

void DownloadEngine::startNextDownload(const DownloadTaskInfo& taskInfo)
{
	DownloadItem* downloadItem = new DownloadItem(taskInfo, m_networkManager, this);
	m_activeDownloads[taskInfo.taskId] = downloadItem;
	m_currentDownloads++;

	connect(downloadItem, &DownloadItem::finished,
		this, &DownloadEngine::onDownloadItemFinished);
	connect(downloadItem, &DownloadItem::progress,
		this, &DownloadEngine::downloadProgress);

	downloadItem->start();
	emit downloadStarted(taskInfo.taskId);
}

void DownloadEngine::onDownloadItemFinished(const QString& taskId)
{
	DownloadItem* item = m_activeDownloads.value(taskId);
	if (item) {
		if (item->status() == Completed) {
			emit downloadCompleted(taskId, m_allTasks[taskId].request.outputPath);
		}
		else if (item->status() == Failed) {
			emit downloadFailed(taskId, "下载失败");
		}

		cleanupDownload(taskId);

		// 下载完成后立即处理队列，开始新的下载
		processQueue();
	}
}

void DownloadEngine::cleanupDownload(const QString& taskId)
{
	if (m_activeDownloads.contains(taskId)) {
		DownloadItem* item = m_activeDownloads.take(taskId);
		item->deleteLater();
		m_currentDownloads--;
	}
	m_allTasks.remove(taskId);
}

// DownloadItem 实现
DownloadItem::DownloadItem(const DownloadTaskInfo& taskInfo, QNetworkAccessManager* manager, QObject* parent)
	: QObject(parent)
	, m_taskInfo(taskInfo)
	, m_networkManager(manager)
	, m_reply(nullptr)
	, m_file(nullptr)
	, m_downloadedBytes(0)
	, m_totalBytes(0)
	, m_isPaused(false)
	, m_isCanceled(false)
{
}

DownloadItem::~DownloadItem()
{
	cleanup();
}

void DownloadItem::start()
{
	if (m_isCanceled) return;

	// 创建目录
	QFileInfo fileInfo(m_taskInfo.request.outputPath);
	QDir dir = fileInfo.absoluteDir();
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	// 打开文件
	m_file = new QFile(m_taskInfo.request.outputPath, this);
	if (!m_file->open(QIODevice::WriteOnly)) {
		m_taskInfo.status = Failed;
		emit finished(m_taskInfo.taskId);
		return;
	}

	// 创建网络请求
	QNetworkRequest request;
	request.setUrl(m_taskInfo.request.videoPlayUrl);

	request.setRawHeader("User-Agent",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
	request.setRawHeader("Referer", "https://www.bilibili.com");
	request.setRawHeader("Origin", "https://www.bilibili.com");

	m_reply = m_networkManager->get(request);

	connect(m_reply, &QNetworkReply::readyRead, this, &DownloadItem::onReadyRead);
	connect(m_reply, &QNetworkReply::finished, this, &DownloadItem::onFinished);
	connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadItem::onDownloadProgress);
	connect(m_reply, &QNetworkReply::errorOccurred, this, &DownloadItem::onErrorOccurred);

	m_taskInfo.status = Downloading;
}

void DownloadItem::pause()
{
	if (m_reply && m_reply->isRunning()) {
		m_reply->abort();
		m_isPaused = true;
		m_taskInfo.status = Paused;
	}
}

void DownloadItem::resume()
{
	if (m_isPaused && !m_isCanceled) {
		start();
		m_isPaused = false;
	}
}

void DownloadItem::cancel()
{
	m_isCanceled = true;
	if (m_reply) {
		m_reply->abort();
	}
	cleanup();
	m_taskInfo.status = DownloadStatus::Cancelled;
}

void DownloadItem::onReadyRead()
{
	if (m_file && m_reply) {
		QByteArray data = m_reply->readAll();
		m_file->write(data);
		m_downloadedBytes += data.size();
	}
}

void DownloadItem::onFinished()
{
	if (m_file) {
		m_file->close();
	}

	if (m_reply->error() == QNetworkReply::NoError && !m_isCanceled && !m_isPaused) {
		m_taskInfo.status = Completed;
	}
	else if (m_isPaused) {
		// 暂停状态，不发出完成信号
		return;
	}
	else {
		m_taskInfo.status = Failed;
		if (m_file) {
			m_file->remove(); // 删除不完整的文件
		}
	}

	emit finished(m_taskInfo.taskId);
}

void DownloadItem::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_downloadedBytes = bytesReceived;
	m_totalBytes = bytesTotal;

	emit progress(m_taskInfo.taskId, bytesReceived, bytesTotal);
}

void DownloadItem::onErrorOccurred(QNetworkReply::NetworkError error)
{
	Q_UNUSED(error)
		if (!m_isPaused && !m_isCanceled) {
			m_taskInfo.status = Failed;
			emit finished(m_taskInfo.taskId);
		}
}

void DownloadItem::cleanup()
{
	if (m_reply) {
		m_reply->deleteLater();
		m_reply = nullptr;
	}
	if (m_file) {
		if (m_file->isOpen()) {
			m_file->close();
		}
		m_file->deleteLater();
		m_file = nullptr;
	}
}