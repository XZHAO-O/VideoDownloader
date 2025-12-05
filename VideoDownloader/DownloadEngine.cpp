#include "DownloadEngine.h"

#include <list>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QTimer>
#include <QMetaObject>

#include "ConfigManager.h"
#include "NetworkManager.h"
#include "DownloadRecordService.h"
#include "StringUtil.h"

DownloadEngine::DownloadEngine(QSharedPointer<ConfigManager> configManager, QSharedPointer<NetworkManager> networkManager, QSharedPointer<DownloadRecordService> downloadRecordService, QWidget* parent)
	: QWidget(parent)
	, m_configManager(configManager)
	, m_networkManager(networkManager)
	, m_downloadRecordService(downloadRecordService)
	, m_maxCurrentDownloads(5)
	, m_maxThreadsPerDownload(3)
	, m_maxDownloadSpeed(10)
	, m_downloadTimer(new QTimer(this))
{
	m_downloadTimer->setInterval(500);
	connect(m_downloadTimer, &QTimer::timeout, this, &DownloadEngine::processDownloadingTasks);
}

DownloadEngine::~DownloadEngine()
{
	for (auto it = m_downloadingTasks.begin(); it != m_downloadingTasks.end(); ++it)
	{
		auto task = *it;

		if (task->status == DownloadStatus::Downloading)
		{
			if (task->isCompleted() && task->downloadFormat != DownloadFormat::Merged)
			{
				//保存下载记录
				m_downloadRecordService->insertOne(task);
			}
			task->pauseDownload(Qt::BlockingQueuedConnection);
			//保存下载任务
		}
	}
}

void DownloadEngine::addDownloadTask(QSharedPointer<DownloadTaskInfo> task)
{
	m_queuedTasks.insert(task->taskId, task);
	if (!m_downloadTimer->isActive())
		m_downloadTimer->start();
	startDownload();
}

void DownloadEngine::pauseDownload(const QString& taskId)
{
	auto downloadingTask = m_downloadingTasks.find(taskId);
	if (downloadingTask != m_downloadingTasks.end())
	{
		auto task = *downloadingTask;
		if (task->status != DownloadStatus::Downloading)
			return;
		task->status = DownloadStatus::Paused;

		task->pauseDownload(Qt::BlockingQueuedConnection);

		m_downloadingTasks.erase(downloadingTask);
		m_pausedTasks.insert(task->taskId, task);  // 放入暂停队列

		endDownloadContext(task);
		task->save();
		startDownload();
	}
	else
	{
		auto queuedTask = m_queuedTasks.find(taskId);
		// 如果任务在等待队列中，也移到暂停队列
		if (queuedTask != m_queuedTasks.end())
		{
			auto task = queuedTask.value();
			if (task->status != DownloadStatus::Downloading)
				return;
			task->status = DownloadStatus::Paused;
			m_queuedTasks.erase(queuedTask);
			m_pausedTasks.insert(task->taskId, task);  // 放入暂停队列
		}
	}
}

void DownloadEngine::resumeDownload(const QString& taskId)
{
	// 从暂停队列中恢复任务
	auto pausedTask = m_pausedTasks.find(taskId);
	if (pausedTask != m_pausedTasks.end())
	{
		auto task = pausedTask.value();
		if (task->status != DownloadStatus::Paused && task->status != DownloadStatus::Failed)
			return;
		task->status = DownloadStatus::Queued;

		m_pausedTasks.erase(pausedTask);
		m_queuedTasks.insert(task->taskId, task);  // 放入等待队列末尾

		// 如果定时器没有启动，启动它
		if (!m_downloadTimer->isActive())
			m_downloadTimer->start();

		startDownload();  // 尝试开始下载
	}
}

void DownloadEngine::cancelDownload(const QString& taskId)
{
	auto downloadingTask = m_downloadingTasks.find(taskId);
	if (downloadingTask != m_downloadingTasks.end())
	{
		auto task = *downloadingTask;
		task->cancelDownload();

		m_downloadingTasks.erase(downloadingTask);
		endDownloadContext(task);
		startDownload();
	}
	else
	{
		// 从等待队列中取消
		m_queuedTasks.remove(taskId);
		// 从暂停队列中取消
		m_pausedTasks.remove(taskId);
	}
}

void DownloadEngine::setMaxCurrentDownloads(int maxCurrentDownloads)
{
}

void DownloadEngine::setMaxThreadsPerDownload(int maxThreadsPerDownload)
{
}

void DownloadEngine::setMaxDownloadSpeed(int maxDownloadSpeed)
{
}

void DownloadEngine::startDownload()
{
	while (m_queuedTasks.size() > 0 && m_downloadingTasks.size() < m_maxCurrentDownloads)
	{
		auto it = m_queuedTasks.begin();
		auto task = it.value();

		m_queuedTasks.erase(it);
		m_downloadingTasks.insert(task->taskId, task);

		// 根据下载格式分配线程
		switch (task->downloadFormat)
		{
		case DownloadFormat::VideoOnly:
			if (!task->videoContext)
				task->createVideoContext();
			allocateAndStartForVideo(task);
			break;

		case DownloadFormat::AudioOnly:
			if (!task->audioContext)
				task->createAudioContext();
			allocateAndStartForAudio(task);
			break;

		case DownloadFormat::Separated:
		case DownloadFormat::Merged:
			if (!task->videoContext)
				task->createVideoContext();
			if (!task->audioContext)
				task->createAudioContext();

			if (task->videoContext->downloadStatus != DownloadStatus::Completed)
			{
				allocateAndStartForVideo(task);
			}
			else if (task->audioContext->downloadStatus != DownloadStatus::Completed)
			{
				allocateAndStartForAudio(task);
			}
			break;
		}
	}
}

void DownloadEngine::processDownloadingTasks()
{
	QList<QString> completedTasks;

	for (auto it = m_downloadingTasks.begin(); it != m_downloadingTasks.end(); ++it)
	{
		auto task = *it;

		if (task->status == DownloadStatus::Downloading)
		{
			bool isCompleted = false;
			bool isFailed = false;
			switch (task->downloadFormat)
			{
			case DownloadFormat::VideoOnly:
				switch (task->videoContext->downloadStatus)
				{
				case DownloadStatus::Completed:
					isCompleted = true;
					endVideoContext(task);
					break;

				case DownloadStatus::Failed:
					isFailed = true;
					endVideoContext(task);
					break;
				}
				break;

			case DownloadFormat::AudioOnly:
				switch (task->audioContext->downloadStatus)
				{
				case DownloadStatus::Completed:
					isCompleted = true;
					endAudioContext(task);
					break;

				case DownloadStatus::Failed:
					isFailed = true;
					endAudioContext(task);
					break;
				}
				break;

			case DownloadFormat::Separated:
			case DownloadFormat::Merged:
				switch (task->videoContext->downloadStatus)
				{
				case DownloadStatus::Completed:
					if (task->audioContext->downloadStatus == DownloadStatus::Completed)
					{
						isCompleted = true;
						endVideoContext(task);
						endAudioContext(task);
					}
					else if (task->audioContext->downloadStatus == DownloadStatus::Failed)
					{
						isFailed = true;
						endVideoContext(task);
						endAudioContext(task);
					}
					else if (task->downloadPeriod == DownloadPeriod::Video)
					{
						endVideoContext(task);
						endAudioContext(task);
						allocateAndStartForAudio(task);
					}
					break;

				case DownloadStatus::Failed:
					isFailed = true;
					endVideoContext(task);
					endAudioContext(task);
					break;
				}
				break;
			}

			if (isCompleted)
			{
				completedTasks.append(task->taskId);
				processCompletedTasks(task);
				continue;
			}
			else if (isFailed)
			{
				completedTasks.append(task->taskId);
				processFailedTasks(task);
				continue;
			}

			QString progressInfo;
			int progress;
			QString downloadSpeed;
			task->formatDownloadInfo(progress, progressInfo, downloadSpeed);
			emit downloadProgress(task->taskId, progressInfo, progress, downloadSpeed);
		}
	}

	// 在循环外移除已完成任务
	for (const QString& taskId : completedTasks)
	{
		m_downloadingTasks.remove(taskId);
	}

	if (!completedTasks.isEmpty())
	{
		startDownload();
	}
}

void DownloadEngine::allocateAndStartForVideo(QSharedPointer<DownloadTaskInfo> task)
{
	m_downloadThreadPool.allocateThread(task->videoContext);
	task->downloadPeriod = DownloadPeriod::Video;
	QMetaObject::invokeMethod(task->videoContext, [this, task]() {
		task->videoContext->startDownload(m_networkManager);
		}, Qt::QueuedConnection);
}

void DownloadEngine::allocateAndStartForAudio(QSharedPointer<DownloadTaskInfo> task)
{
	m_downloadThreadPool.allocateThread(task->audioContext);
	task->downloadPeriod = DownloadPeriod::Audio;
	QMetaObject::invokeMethod(task->audioContext, [this, task]() {
		task->audioContext->startDownload(m_networkManager);
		}, Qt::QueuedConnection);
}

void DownloadEngine::processCompletedTasks(QSharedPointer<DownloadTaskInfo> task)
{
	if (task->downloadFormat != DownloadFormat::Merged)
	{
		//保存下载记录
		m_downloadRecordService->insertOne(task);
		emit downloadFinished(task->taskId);
		return;
	}
	//音视频合流
}

void DownloadEngine::processFailedTasks(QSharedPointer<DownloadTaskInfo> task)
{
	//endDownloadContext(task);
}

void DownloadEngine::endVideoContext(QSharedPointer<DownloadTaskInfo> task)
{
	m_downloadThreadPool.releaseThread(task->videoContext);
	disconnect(task->videoContext, nullptr, this, nullptr);
}

void DownloadEngine::endAudioContext(QSharedPointer<DownloadTaskInfo> task)
{
	m_downloadThreadPool.releaseThread(task->audioContext);
	disconnect(task->audioContext, nullptr, this, nullptr);
}

void DownloadEngine::endDownloadContext(QSharedPointer<DownloadTaskInfo> task)
{
	// 根据下载格式释放相应的线程
	switch (task->downloadFormat)
	{
	case DownloadFormat::Merged:
	case DownloadFormat::VideoOnly:
		endVideoContext(task);
		break;
	case DownloadFormat::AudioOnly:
		endAudioContext(task);
		break;
	case DownloadFormat::Separated:
		endVideoContext(task);
		endAudioContext(task);
		break;
	}
}