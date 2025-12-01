#include "DownloadEngine.h"

#include <list>
#include <QMutexLocker>
#include <QtConcurrent>
#include <QTimer>
#include <QMetaObject>

#include "ConfigManager.h"
#include "NetworkManager.h"
#include "StringUtil.h"

DownloadEngine::DownloadEngine(QSharedPointer<ConfigManager> configManager, QSharedPointer<NetworkManager> networkManager, QWidget* parent)
	: QWidget(parent)
	, m_configManager(configManager)
	, m_networkManager(networkManager)
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
	for (auto task : m_downloadingTasks)
	{
		task->pauseDownload(Qt::BlockingQueuedConnection);
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
		task->status = DownloadStatus::Paused;

		task->pauseDownload();

		m_downloadingTasks.erase(downloadingTask);
		m_queuedTasks.insert(task->taskId, task);

		endDownloadContext(task);
		startDownload();
	}
	else
	{
		if (m_queuedTasks.contains(taskId))
		{
			auto task = m_queuedTasks.take(taskId);
			task->status = DownloadStatus::Paused;
			m_queuedTasks.remove(task->taskId);
			m_queuedTasks.insert(task->taskId, task);
		}
	}
}

void DownloadEngine::resumeDownload(const QString& taskId)
{
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
		m_queuedTasks.remove(taskId);
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

		if (task->status == DownloadStatus::Paused)
			break;

		m_queuedTasks.erase(it);
		m_downloadingTasks.insert(task->taskId, task);

		// 创建 download context
		if (!task->videoContext && !task->audioContext)
			task->createContext();

		// 根据下载格式分配线程
		switch (task->downloadFormat)
		{
		case DownloadFormat::Merged:
		case DownloadFormat::VideoOnly:
			if (task->videoContext)
				m_downloadThreadPool.allocateThread(task->videoContext);
			break;
		case DownloadFormat::AudioOnly:
			if (task->audioContext)
				m_downloadThreadPool.allocateThread(task->audioContext);
			break;
		case DownloadFormat::Separated:
			if (task->videoContext)
				m_downloadThreadPool.allocateThread(task->videoContext);
			break;
		}

		task->startDownload(m_networkManager);
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
			if (task->isCompleted())
			{
				completedTasks.append(task->taskId);
				endDownloadContext(task);
				emit downloadFinished(task->taskId);
				continue;
			}

			// 处理 Separated 格式的音频下载切换
			if (task->downloadFormat == DownloadFormat::Separated &&
				task->videoContext &&
				task->videoContext->downloadStatus == DownloadStatus::Completed &&
				task->downloadPeriod != DownloadPeriod::Audio)
			{
				task->downloadPeriod = DownloadPeriod::Audio;

				// 为音频下载分配线程
				if (task->audioContext)
					m_downloadThreadPool.allocateThread(task->audioContext);

				QMetaObject::invokeMethod(task->audioContext, [this, task]() {
					task->audioContext->startDownload(m_networkManager);
					}, Qt::QueuedConnection);
			}

			if (task->isFailed())
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

void DownloadEngine::processFailedTasks(QSharedPointer<DownloadTaskInfo> task)
{
	endDownloadContext(task);
}

void DownloadEngine::endDownloadContext(QSharedPointer<DownloadTaskInfo> task)
{
	// 根据下载格式释放相应的线程
	switch (task->downloadFormat)
	{
	case DownloadFormat::Merged:
	case DownloadFormat::VideoOnly:
		if (task->videoContext)
		{
			m_downloadThreadPool.releaseThread(task->videoContext);
			task->disconnectContext(this);
		}
		break;
	case DownloadFormat::AudioOnly:
		if (task->audioContext)
		{
			m_downloadThreadPool.releaseThread(task->audioContext);
			task->disconnectContext(this);
		}
		break;
	case DownloadFormat::Separated:
		if (task->videoContext)
		{
			m_downloadThreadPool.releaseThread(task->videoContext);
		}
		if (task->audioContext)
		{
			m_downloadThreadPool.releaseThread(task->audioContext);
		}
		task->disconnectContext(this);
		break;
	}
}