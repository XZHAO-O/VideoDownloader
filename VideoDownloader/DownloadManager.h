#pragma once

#include <QObject>
#include <QMap>
#include <QQueue>
#include <QSharedPointer>
#include "DownloadTaskInfo.h"
#include "VideoDownloadRequest.h"

// 前向声明，避免循环依赖
class ApplicationController;

class DownloadManager : public QObject
{
	Q_OBJECT

public:
	explicit DownloadManager(QSharedPointer<ApplicationController> appController,
		QObject* parent = nullptr);

	// 下载操作
	QString downloadVideo(const VideoDownloadRequest& request);
	void pauseDownload(const QString& taskId);
	void resumeDownload(const QString& taskId);
	void cancelDownload(const QString& taskId);

	void calculateAndEmitDownloadSpeed();

	void updateDownloadSpeed(qint64 bytesPerSecond);

	// 批量操作
	QList<QString> downloadBatch(const QList<VideoDownloadRequest>& requests);
	void pauseAll();
	void resumeAll();
	void cancelAll();

	// 查询操作
	QList<DownloadTaskInfo> getActiveDownloads() const;
	QList<DownloadTaskInfo> getCompletedDownloads() const;
	QList<DownloadTaskInfo> getQueuedDownloads() const;
	DownloadTaskInfo getDownloadInfo(const QString& taskId) const;

	// 设置
	void setMaxConcurrentDownloads(int count);
	int getMaxConcurrentDownloads() const;
	void setDefaultDownloadPath(const QString& path);
	QString getDefaultDownloadPath() const;

signals:
	void downloadAdded(const QString& taskId);
	void downloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void downloadCompleted(const QString& taskId, const QString& filePath);
	void downloadFailed(const QString& taskId, const QString& error);
	void downloadPaused(const QString& taskId);
	void downloadResumed(const QString& taskId);
	void downloadCancelled(const QString& taskId);

	// 添加缺失的信号
	void downloadStatusChanged(const QString& taskId);
	void downloadSpeedUpdated(qint64 bytesPerSecond);

private slots:
	void onOrchestrationProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void onOrchestrationCompleted(const QString& taskId, const QString& filePath);
	void onOrchestrationFailed(const QString& taskId, const QString& error);

private:
	void processQueue();
	void startNextDownload();
	void updateDownloadInfo(const QString& taskId, const DownloadTaskInfo& info);
	void completeDownload(const QString& taskId, bool success, const QString& filePath = "");

	QSharedPointer<ApplicationController> m_appController;
	QMap<QString, DownloadTaskInfo> m_activeDownloads;
	QMap<QString, DownloadTaskInfo> m_completedDownloads;
	QMap<QString, DownloadTaskInfo> m_queuedDownloads;
	QQueue<QString> m_downloadQueue;
	int m_maxConcurrentDownloads = 3;
	int m_currentDownloads = 0;
	QString m_defaultDownloadPath;
	QTimer* m_speedTimer;
};