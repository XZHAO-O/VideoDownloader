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
	startDownload();
}

void DownloadEngine::pauseDownload(const QString& taskId)
{
	auto downloadingTask = m_downloadingTasks.find(taskId);
	if (downloadingTask != m_downloadingTasks.end())
	{
		auto task = *downloadingTask;
		m_downloadThreadPool.releaseThread(&(task->context));
		//downloadingTask.downloadContext.pause();
		task->context.moveToThread(QThread::currentThread());

	}
	else
	{
		if (m_tasks.contains(taskId))
		{
			auto task = m_tasks[taskId];
			//task->isPaused = true;
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
		//downloadingTask.downloadContext.cancel();
		endDownloadContext(task);
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

		auto context = &(task->context);
		m_downloadThreadPool.allocateThread(context);
		//connect(context, &DownloadContext::downloadFinished, this, &DownloadEngine::processDownloadingTasks);
		//QMetaObject::invokeMethod(context, "startDownload");

	}
}

void DownloadEngine::processDownloadingTasks()
{
	for (auto it = m_downloadingTasks.begin(); it != m_downloadingTasks.end(); ++it)
	{
		auto task = *it;
		switch (task->status)
		{
		case DownloadStatus::Downloading:
			//StringUtil::formatDownloadProgress(task->context.downloadedSize, task->context.fileSize);
			//StringUtil::formatDownloadSpeed(task->context.downloadedSize - task->context.progressedSize);
			//task->context.progressedSize = task->context.downloadedSize;
			break;
		case DownloadStatus::Completed:
			endDownloadContext(task);
			m_downloadingTasks.erase(it);
			emit downloadFinished(task->taskId);
			startDownload();
			break;
		case DownloadStatus::Failed:
			processFailedTasks(task);
			m_downloadingTasks.erase(it);
			break;
		}
	}
}

void DownloadEngine::processFailedTasks(QSharedPointer<DownloadTaskInfo> task)
{
	endDownloadContext(task);
}

void DownloadEngine::endDownloadContext(QSharedPointer<DownloadTaskInfo> task)
{
	m_downloadThreadPool.releaseThread(&(task->context));
	task->context.moveToThread(QThread::currentThread());
	disconnect(&(task->context), nullptr, this, nullptr);
}
