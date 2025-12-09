#pragma once

#include <QWidget>
#include <QSharedPointer>
#include <QMutex>
#include <QQueue>

#include "DownloadTaskInfo.h"
#include "DownloadThreadPool.h"
#include "OrderedQHash.h"

class QTimer;
class ConfigManager;
class NetworkManager;
class DownloadRecordService;
class DownloadVideoCoverService;

class DownloadEngine : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadEngine(QSharedPointer<ConfigManager> configManager, QSharedPointer<NetworkManager> networkManager, QSharedPointer<DownloadRecordService> downloadRecordService, QSharedPointer<DownloadVideoCoverService> downloadVideoCoverService, QWidget* parent = nullptr);
	~DownloadEngine();

	void addDownloadTask(QSharedPointer<DownloadTaskInfo> task);
	void pauseDownload(const QString& taskId);
	void resumeDownload(const QString& taskId);
	void cancelDownload(const QString& taskId);

	void setMaxCurrentDownloads(int maxCurrentDownloads);
	void setMaxThreadsPerDownload(int maxThreadsPerDownload);
	void setMaxDownloadSpeed(int maxDownloadSpeed);

signals:
	void downloadProgress(const QString& taskId, const QString& progressInfo, int progress, const QString& downloadSpeed);
	void downloadFinished(const QString& taskId);
	void downloadFailed(const QString& error);

private:
	void startDownload();

	void processDownloadingTasks();

	void allocateAndStartForVideo(QSharedPointer<DownloadTaskInfo> task);
	void allocateAndStartForAudio(QSharedPointer<DownloadTaskInfo> task);

	void processCompletedTasks(QSharedPointer<DownloadTaskInfo> task);
	void processFailedTasks(QSharedPointer<DownloadTaskInfo> task);

	void endVideoContext(QSharedPointer<DownloadTaskInfo> task);
	void endAudioContext(QSharedPointer<DownloadTaskInfo> task);
	void endDownloadContext(QSharedPointer<DownloadTaskInfo> task);

	QSharedPointer<ConfigManager> m_configManager;
	QSharedPointer<NetworkManager> m_networkManager;
	QSharedPointer<DownloadRecordService> m_downloadRecordService;
	QSharedPointer<DownloadVideoCoverService> m_downloadVideoCoverService;

	DownloadThreadPool<DownloadContext*> m_downloadThreadPool;
	OrderedQHash<const QString, QSharedPointer<DownloadTaskInfo>> m_queuedTasks;    // 等待队列
	QHash<const QString, QSharedPointer<DownloadTaskInfo>> m_pausedTasks;          // 暂停队列
	QHash<const QString, QSharedPointer<DownloadTaskInfo>> m_downloadingTasks;     // 正在下载的队列

	int m_maxCurrentDownloads = 5;
	int m_maxThreadsPerDownload = 5;
	int m_maxDownloadSpeed = -1;

	QTimer* m_downloadTimer;
	QMutex m_mutex;
};