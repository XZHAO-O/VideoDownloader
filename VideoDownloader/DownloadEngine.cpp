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
	m_downloadTimer->setInterval(1000);
	connect(m_downloadTimer, &QTimer::timeout, this, &DownloadEngine::processDownloadingTasks);
}

DownloadEngine::~DownloadEngine()
{

}

void DownloadEngine::addDownloadTask(QSharedPointer<DownloadTaskInfo> task)
{
	m_queuedTasks.push_back(task);
	m_tasks.insert(task->taskId, m_queuedTasks.end());
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

		QMetaObject::invokeMethod(task->context, "stopDownload");

		m_downloadingTasks.erase(downloadingTask);
		m_queuedTasks.push_back(task);
		m_tasks.insert(task->taskId, m_queuedTasks.end());

		endDownloadContext(task);
		startDownload();
	}
	else
	{
		if (m_tasks.contains(taskId))
		{
			auto task = m_tasks[taskId];
			(*task)->status = DownloadStatus::Paused;
			m_queuedTasks.erase(task);
			m_queuedTasks.push_back(*task);
			m_tasks[taskId] = m_queuedTasks.end();
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
		QMetaObject::invokeMethod(task->context, "stopDownload");
		m_downloadingTasks.erase(downloadingTask);
		endDownloadContext(task);
		startDownload();
	}
	else
	{
		auto task = m_tasks.find(taskId);
		if (task != m_tasks.end())
		{
			m_queuedTasks.erase(*task);
			m_tasks.erase(task);
		}
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
		auto task = m_queuedTasks.front();
		if (task->status == DownloadStatus::Paused)
			break;
		m_queuedTasks.pop_front();
		m_tasks.remove(task->taskId);
		m_downloadingTasks.insert(task->taskId, task);

		//创建downloadcontext
		if (!task->context)
			task->createContext();
		m_downloadThreadPool.allocateThread(task->context);
		QMetaObject::invokeMethod(task->context, "startDownload", m_networkManager);
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
			if (task->context->downloadStatus == DownloadStatus::Completed)
			{
				completedTasks.append(task->taskId);
				endDownloadContext(task);
				emit downloadFinished(task->taskId);
				continue;
			}

			if (task->context->downloadStatus == DownloadStatus::Failed)
			{
				completedTasks.append(task->taskId);
				processFailedTasks(task);
				continue;
			}

			qint64 downloadedBytes = task->context->downloadedTotalSize;
			QString progressInfo = StringUtil::formatDownloadProgress(downloadedBytes, task->context->fileSize);
			int progress = downloadedBytes * 100 / task->context->fileSize;
			QString downloadSpeed = StringUtil::formatDownloadSpeed(downloadedBytes - task->context->progressedSize);
			task->context->progressedSize = downloadedBytes;
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
	m_downloadThreadPool.releaseThread(task->context);
	//task->context->moveToThread(QThread::currentThread());
	disconnect(task->context, nullptr, this, nullptr);
}
