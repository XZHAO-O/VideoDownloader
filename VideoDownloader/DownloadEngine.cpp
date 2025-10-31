#include "DownloadEngine.h"

#include <QMutexLocker>

#include "ConfigManager.h"
#include "NetworkManager.h"

DownloadEngine::DownloadEngine(QSharedPointer<ConfigManager> configManager, QSharedPointer<NetworkManager> networkManager, QWidget* parent)
	: QWidget(parent)
	, m_configManager(configManager)
	, m_networkManager(networkManager)
	, m_maxCurrentDownloads(5)
	, m_maxThreadsPerDownload(3)
	, m_maxDownloadSpeed(10)
{
}

DownloadEngine::~DownloadEngine()
{

}

void DownloadEngine::addDownloadTask(QSharedPointer<DownloadTaskInfo> task)
{
	m_tasks.insert(task->taskId, task);
	startDownload();
}

void DownloadEngine::pauseDownload(const QString& taskId)
{
}

void DownloadEngine::resumeDownload(const QString& taskId)
{
}

void DownloadEngine::cancelDownload(const QString& taskId)
{
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

void DownloadEngine::onDownloadFinished()
{

}

void DownloadEngine::onDownloadFailed(const QString& error)
{
}

void DownloadEngine::onDownloadProgress(const QString& progress)
{
}

void DownloadEngine::initConnections()
{
	/*connect(this, &NetworkManager::downloadFinished, this, &DownloadEngine::onDownloadFinished);*/
}

void DownloadEngine::startDownload()
{
	QMutexLocker lock(&m_mutex);
	while (m_tasks.size() > 0 && m_downloadingTasks.size() < m_maxCurrentDownloads)
	{

	}
}
