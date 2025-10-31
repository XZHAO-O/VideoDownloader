#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QMutex>

#include "DownloadTaskInfo.h"

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
	void downloadFinished();
	void downloadFailed(const QString& error);

private slots:
	void onDownloadFinished();
	void onDownloadFailed(const QString& error);
	void onDownloadProgress(const QString& progress);

private:

	void initConnections();

	void startDownload();

	QSharedPointer<ConfigManager> m_configManager;
	QSharedPointer<NetworkManager> m_networkManager;
	QMap<const QString, QSharedPointer<DownloadTaskInfo>> m_tasks;
	QMap<const QString, QSharedPointer<DownloadTaskInfo>> m_downloadingTasks;

	int m_maxCurrentDownloads;
	int m_maxThreadsPerDownload;
	int m_maxDownloadSpeed;

	QMutex m_mutex;
};