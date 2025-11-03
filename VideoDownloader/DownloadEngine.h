#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QMutex>
#include <QQueue>

#include "DownloadTaskInfo.h"

class QTimer;
class ConfigManager;
class NetworkManager;

class DownloadEngine : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadEngine(QSharedPointer<ConfigManager> configManager, QSharedPointer<NetworkManager> networkManager, QWidget* parent = nullptr);
	~DownloadEngine();

	void addDownloadTask(QSharedPointer<DownloadTaskInfo> task);
	void pauseDownload(const QString& taskId);
	void resumeDownload(const QString& taskId);
	void cancelDownload(const QString& taskId);

	void setMaxCurrentDownloads(int maxCurrentDownloads);
	void setMaxThreadsPerDownload(int maxThreadsPerDownload);
	void setMaxDownloadSpeed(int maxDownloadSpeed);

signals:
	void downloadProgress(const QString& progress);
	void downloadFinished(const QString& taskId);
	void downloadFailed(const QString& error);

private:
	void startDownload();

	void processDownloadingTasks();
	void processFailedTasks(QSharedPointer<DownloadTaskInfo> task);

	QSharedPointer<ConfigManager> m_configManager;
	QSharedPointer<NetworkManager> m_networkManager;
	QHash<const QString, std::list<QSharedPointer<DownloadTaskInfo>>::iterator> m_tasks;
	std::list<QSharedPointer<DownloadTaskInfo>> m_queuedTasks;
	QHash<const QString, QSharedPointer<DownloadTaskInfo>> m_downloadingTasks;

	int m_maxCurrentDownloads;
	int m_maxThreadsPerDownload;
	int m_maxDownloadSpeed;

	QTimer* m_downloadTimer;
	QMutex m_mutex;
};