#include "DownloadEngine.h"

#include <QDir>
#include <QRandomGenerator>
#include <QThreadPool>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include "ApplicationState.h"
#include "LogSystem.h"
#include "ConfigManager.h"

DownloadEngine::DownloadEngine(QSharedPointer<ConfigManager> configManager, QObject* parent)
	: IDownloadEngine(parent)  // 现在可以正确调用基类构造函数
	, m_configManager(configManager)
	, m_networkManager(new QNetworkAccessManager(this))
	, m_threadPool(new QThreadPool(this))
	, m_queueTimer(new QTimer(this))
	, m_speedTimer(new QTimer(this))
	, m_maxConcurrentDownloads(3)
	, m_downloadSpeedLimit(0)
	, m_currentTotalSpeed(0)
{
	// 使用 LogSystem 单例
	m_logger = &LogSystem::instance();

	// 设置线程池
	m_threadPool->setMaxThreadCount(m_maxConcurrentDownloads);

	// 设置队列处理定时器
	connect(m_queueTimer, &QTimer::timeout, this, &DownloadEngine::processTaskQueue);
	m_queueTimer->start(1000); // 每秒检查一次队列

	// 设置速度统计定时器
	connect(m_speedTimer, &QTimer::timeout, this, &DownloadEngine::updateSpeedStatistics);
	m_speedTimer->start(1000); // 每秒更新一次速度统计

	LOG_INFO("DownloadEngine", "DownloadEngine initialized with " + QString::number(m_maxConcurrentDownloads) + " max concurrent downloads");
}

DownloadEngine::~DownloadEngine()
{
	m_queueTimer->stop();
	m_speedTimer->stop();

	// 停止所有活动下载
	QMutexLocker locker(&m_tasksMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->status == Downloading && it->reply) {
			it->reply->abort();
			it->reply->deleteLater();
		}
		if (it->file && it->file->isOpen()) {
			it->file->close();
			delete it->file;
		}
	}
	m_tasks.clear();
}

QString DownloadEngine::addTask(const DownloadTask& task)
{
	if (!task.url.isValid() || task.savePath.isEmpty()) {
		LOG_ERROR("DownloadEngine", "Invalid download task: URL=" + task.url.toString() + ", SavePath=" + task.savePath);
		return QString();
	}

	QString taskId = task.taskId.isEmpty() ?
		QString("download_%1_%2").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
		.arg(QRandomGenerator::global()->generate() % 10000)
		: task.taskId;

	DownloadTaskItem taskItem;
	taskItem.task = task;
	taskItem.task.taskId = taskId;
	taskItem.status = Queued;
	taskItem.startTime = QDateTime::currentDateTime();

	{
		QMutexLocker locker(&m_tasksMutex);
		m_tasks[taskId] = taskItem;
		m_taskQueue.enqueue(taskId);
	}

	LOG_INFO("DownloadEngine", "Download task added: " + taskId + " -> " + task.url.toString());

	emit taskStatusChanged(taskId, Queued);

	return taskId;
}

bool DownloadEngine::removeTask(const QString& taskId)
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		LOG_WARN("DownloadEngine", "Task not found for removal: " + taskId);
		return false;
	}

	DownloadTaskItem& task = m_tasks[taskId];

	// 如果任务正在下载，先停止
	if (task.status == Downloading && task.reply) {
		task.reply->abort();
	}

	// 清理文件
	if (task.file && task.file->isOpen()) {
		task.file->close();
		if (task.status != Completed) {
			task.file->remove(); // 删除未完成文件
		}
		delete task.file;
		task.file = nullptr;
	}

	m_tasks.remove(taskId);

	// 从队列中移除
	m_taskQueue.removeAll(taskId);

	LOG_INFO("DownloadEngine", "Download task removed: " + taskId);
	return true;
}

bool DownloadEngine::pauseTask(const QString& taskId)
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		LOG_WARN("DownloadEngine", "Task not found for pause: " + taskId);
		return false;
	}

	DownloadTaskItem& task = m_tasks[taskId];

	if (task.status != Downloading) {
		LOG_WARN("DownloadEngine", "Task is not downloading, cannot pause: " + taskId);
		return false;
	}

	if (task.reply) {
		task.reply->abort();
		task.reply->deleteLater();
		task.reply = nullptr;
	}

	if (task.file && task.file->isOpen()) {
		task.file->close();
	}

	task.status = Paused;

	LOG_INFO("DownloadEngine", "Download task paused: " + taskId);
	emit taskStatusChanged(taskId, Paused);

	return true;
}

bool DownloadEngine::resumeTask(const QString& taskId)
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		LOG_WARN("DownloadEngine", "Task not found for resume: " + taskId);
		return false;
	}

	DownloadTaskItem& task = m_tasks[taskId];

	if (task.status != Paused) {
		LOG_WARN("DownloadEngine", "Task is not paused, cannot resume: " + taskId);
		return false;
	}

	// 重新加入队列
	task.status = Queued;
	m_taskQueue.enqueue(taskId);

	LOG_INFO("DownloadEngine", "Download task resumed: " + taskId);
	emit taskStatusChanged(taskId, Queued);

	return true;
}

bool DownloadEngine::cancelTask(const QString& taskId)
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		LOG_WARN("DownloadEngine", "Task not found for cancel: " + taskId);
		return false;
	}

	DownloadTaskItem& task = m_tasks[taskId];

	// 停止下载
	if (task.reply) {
		task.reply->abort();
		task.reply->deleteLater();
		task.reply = nullptr;
	}

	// 删除文件
	if (task.file) {
		if (task.file->isOpen()) {
			task.file->close();
		}
		if (task.status != Completed) {
			task.file->remove();
		}
		delete task.file;
		task.file = nullptr;
	}

	task.status = Cancelled;

	// 从队列中移除
	m_taskQueue.removeAll(taskId);

	LOG_INFO("DownloadEngine", "Download task cancelled: " + taskId);
	emit taskStatusChanged(taskId, Cancelled);

	return true;
}

QList<QString> DownloadEngine::getActiveTasks() const
{
	QMutexLocker locker(&m_tasksMutex);

	QList<QString> activeTasks;
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->status == Downloading || it->status == Queued) {
			activeTasks.append(it.key());
		}
	}

	return activeTasks;
}

bool DownloadEngine::isTaskActive(const QString& taskId) const
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		return false;
	}

	const DownloadTaskItem& task = m_tasks[taskId];
	return task.status == Downloading || task.status == Queued;
}

qint64 DownloadEngine::getTaskProgress(const QString& taskId) const
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		return 0;
	}

	return m_tasks[taskId].downloadedBytes;
}

qint64 DownloadEngine::getTaskTotalSize(const QString& taskId) const
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		return 0;
	}

	return m_tasks[taskId].totalBytes;
}

void DownloadEngine::setMaxConcurrentDownloads(int count)
{
	if (count <= 0) {
		LOG_WARN("DownloadEngine", "Invalid max concurrent downloads count: " + QString::number(count));
		return;
	}

	m_maxConcurrentDownloads = count;
	m_threadPool->setMaxThreadCount(count);

	LOG_INFO("DownloadEngine", "Max concurrent downloads set to: " + QString::number(count));
}

int DownloadEngine::maxConcurrentDownloads() const
{
	return m_maxConcurrentDownloads;
}

void DownloadEngine::setDownloadSpeedLimit(qint64 bytesPerSecond)
{
	m_downloadSpeedLimit = bytesPerSecond;
	LOG_INFO("DownloadEngine", "Download speed limit set to: " + QString::number(bytesPerSecond) + " bytes/s");
}

qint64 DownloadEngine::downloadSpeedLimit() const
{
	return m_downloadSpeedLimit;
}

void DownloadEngine::processTaskQueue()
{
	QMutexLocker locker(&m_tasksMutex);

	// 计算当前正在下载的任务数量
	int currentDownloads = 0;
	for (const auto& task : m_tasks) {
		if (task.status == Downloading) {
			currentDownloads++;
		}
	}

	// 启动新的下载任务直到达到最大并发数
	while (currentDownloads < m_maxConcurrentDownloads && !m_taskQueue.isEmpty()) {
		QString taskId = m_taskQueue.dequeue();

		if (m_tasks.contains(taskId) && m_tasks[taskId].status == Queued) {
			// 在线程池中执行下载任务
			DownloadRunnable* runnable = new DownloadRunnable(this, taskId);
			m_threadPool->start(runnable);
			currentDownloads++;
		}
	}
}

void DownloadEngine::updateSpeedStatistics()
{
	QMutexLocker locker(&m_tasksMutex);

	m_currentTotalSpeed = 0;
	QDateTime currentTime = QDateTime::currentDateTime();

	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->status == Downloading) {
			qint64 speed = calculateCurrentSpeed(*it);
			m_taskSpeeds[it.key()] = speed;
			m_currentTotalSpeed += speed;

			// 更新任务的上次统计时间
			it->lastUpdateTime = currentTime;
			it->lastDownloadedBytes = it->downloadedBytes;
		}
	}

	emit downloadSpeedUpdated(m_currentTotalSpeed);
}

bool DownloadEngine::startDownload(const QString& taskId)
{
	QMutexLocker locker(&m_tasksMutex);

	if (!m_tasks.contains(taskId)) {
		LOG_ERROR("DownloadEngine", "Task not found for start: " + taskId);
		return false;
	}

	DownloadTaskItem& task = m_tasks[taskId];

	try {
		// 准备保存目录
		QFileInfo fileInfo(task.task.savePath);
		QDir dir = fileInfo.dir();
		if (!dir.exists()) {
			if (!dir.mkpath(".")) {
				throw std::runtime_error("Failed to create directory: " + dir.absolutePath().toStdString());
			}
		}

		// 打开文件
		task.file = new QFile(task.task.savePath);
		if (!task.file->open(QIODevice::WriteOnly | QIODevice::Append)) {
			throw std::runtime_error("Failed to open file for writing: " + task.task.savePath.toStdString());
		}

		// 创建网络请求
		QNetworkRequest request(task.task.url);

		// 设置请求头
		for (auto it = task.task.headers.begin(); it != task.task.headers.end(); ++it) {
			request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
		}

		// 设置Range头支持断点续传
		qint64 fileSize = task.file->size();
		if (fileSize > 0) {
			request.setRawHeader("Range", QString("bytes=%1-").arg(fileSize).toUtf8());
			task.downloadedBytes = fileSize;
		}

		// 开始下载
		task.reply = m_networkManager->get(request);
		task.status = Downloading;
		task.startTime = QDateTime::currentDateTime();
		task.lastUpdateTime = task.startTime;
		task.lastDownloadedBytes = task.downloadedBytes;

		// 连接信号槽
		connect(task.reply, &QNetworkReply::downloadProgress,
			this, &DownloadEngine::onDownloadProgress);
		connect(task.reply, &QNetworkReply::finished,
			this, &DownloadEngine::onDownloadFinished);
		connect(task.reply, &QNetworkReply::readyRead,
			this, &DownloadEngine::onReadyRead);

		LOG_INFO("DownloadEngine", "Download started: " + taskId + " -> " + task.task.url.toString());

		emit taskStatusChanged(taskId, Downloading);

		return true;

	}
	catch (const std::exception& e) {
		LOG_ERROR("DownloadEngine", "Failed to start download " + taskId + ": " + QString::fromStdString(e.what()));

		if (task.file && task.file->isOpen()) {
			task.file->close();
			delete task.file;
			task.file = nullptr;
		}

		task.status = Failed;
		task.errorString = QString::fromStdString(e.what());
		emit taskStatusChanged(taskId, Failed);
		emit taskFailed(taskId, task.errorString);

		return false;
	}
}

void DownloadEngine::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	QMutexLocker locker(&m_tasksMutex);

	// 找到对应的任务
	QString taskId;
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->reply == reply) {
			taskId = it.key();
			break;
		}
	}

	if (taskId.isEmpty()) return;

	DownloadTaskItem& task = m_tasks[taskId];

	// 更新进度（考虑已下载的字节数）
	qint64 totalDownloaded = task.downloadedBytes + bytesReceived;
	qint64 totalSize = bytesTotal > 0 ? (task.downloadedBytes + bytesTotal) : bytesTotal;

	task.totalBytes = totalSize;

	// 发射进度信号
	emit taskProgressChanged(taskId, totalDownloaded, totalSize);

	// 更新速度统计
	QDateTime currentTime = QDateTime::currentDateTime();
	if (task.lastUpdateTime.isValid()) {
		qint64 timeDiff = task.lastUpdateTime.msecsTo(currentTime);
		if (timeDiff > 0) {
			qint64 bytesDiff = totalDownloaded - task.lastDownloadedBytes;
			qint64 speed = (bytesDiff * 1000) / timeDiff;
			m_taskSpeeds[taskId] = speed;
		}
	}

	task.lastUpdateTime = currentTime;
	task.lastDownloadedBytes = totalDownloaded;
}

void DownloadEngine::onDownloadFinished()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	QMutexLocker locker(&m_tasksMutex);

	// 找到对应的任务
	QString taskId;
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->reply == reply) {
			taskId = it.key();
			break;
		}
	}

	if (taskId.isEmpty()) return;

	DownloadTaskItem& task = m_tasks[taskId];

	// 检查下载结果
	if (reply->error() == QNetworkReply::NoError) {
		// 下载成功
		if (task.file && task.file->isOpen()) {
			task.file->close();
		}

		task.status = Completed;
		task.downloadedBytes = task.totalBytes;

		LOG_INFO("DownloadEngine", "Download completed: " + taskId + " -> " + task.task.savePath);

		emit taskStatusChanged(taskId, Completed);
		emit taskCompleted(taskId, task.task.savePath);

	}
	else {
		// 下载失败
		QString errorString = reply->errorString();

		LOG_ERROR("DownloadEngine", "Download failed: " + taskId + " - " + errorString);

		// 检查是否需要重试
		if (task.retryCount < task.maxRetries) {
			task.retryCount++;
			LOG_INFO("DownloadEngine", "Retrying download " + taskId + " (attempt " +
				QString::number(task.retryCount) + "/" + QString::number(task.maxRetries) + ")");

			// 重新加入队列
			task.status = Queued;
			m_taskQueue.enqueue(taskId);
			emit taskStatusChanged(taskId, Queued);

		}
		else {
			// 重试次数用完，标记为失败
			task.status = Failed;
			task.errorString = errorString;

			// 清理文件
			if (task.file) {
				if (task.file->isOpen()) {
					task.file->close();
				}
				task.file->remove(); // 删除不完整的文件
				delete task.file;
				task.file = nullptr;
			}

			emit taskStatusChanged(taskId, Failed);
			emit taskFailed(taskId, errorString);
		}
	}

	// 清理网络回复
	if (task.reply) {
		task.reply->deleteLater();
		task.reply = nullptr;
	}
}

void DownloadEngine::onReadyRead()
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	QMutexLocker locker(&m_tasksMutex);

	// 找到对应的任务
	QString taskId;
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->reply == reply) {
			taskId = it.key();
			break;
		}
	}

	if (taskId.isEmpty()) return;

	DownloadTaskItem& task = m_tasks[taskId];

	// 读取数据并写入文件
	if (task.file && task.file->isOpen()) {
		QByteArray data = reply->readAll();
		qint64 bytesWritten = task.file->write(data);

		if (bytesWritten != data.size()) {
			LOG_ERROR("DownloadEngine", "Failed to write all data to file for task: " + taskId);
			// 这里可以触发错误处理
		}

		task.downloadedBytes += bytesWritten;
	}
}

qint64 DownloadEngine::calculateCurrentSpeed(const DownloadTaskItem& task) const
{
	if (!task.lastUpdateTime.isValid() || task.lastUpdateTime == task.startTime) {
		return 0;
	}

	qint64 timeDiff = task.lastUpdateTime.msecsTo(QDateTime::currentDateTime());
	if (timeDiff <= 0) {
		return 0;
	}

	qint64 bytesDiff = task.downloadedBytes - task.lastDownloadedBytes;
	return (bytesDiff * 1000) / timeDiff;
}

// DownloadRunnable 实现
DownloadEngine::DownloadRunnable::DownloadRunnable(DownloadEngine* engine, const QString& taskId)
	: m_engine(engine)
	, m_taskId(taskId)
{
}

void DownloadEngine::DownloadRunnable::run()
{
	m_engine->startDownload(m_taskId);
}