#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QThread>
#include <QMap>
#include <QQueue>
#include "DownloadTaskInfo.h"

class DownloadManager : public QObject
{
	Q_OBJECT

public:
	explicit DownloadManager(QObject* parent = nullptr);
	~DownloadManager();

	// 公共接口
	void addDownload(DownloadTaskInfo taskInfo);
	void pauseDownload(const QString& taskId);
	void resumeDownload(const QString& taskId);
	void cancelDownload(const QString& taskId);

	// 配置设置
	void setMaxConcurrentDownloads(int max);
	void setDownloadSpeedLimit(qint64 bytesPerSecond);
	void setMaxThreadsPerDownload(int maxThreads);

	// 获取任务信息
	DownloadTaskInfo getTaskInfo(const QString& taskId) const;
	QList<DownloadTaskInfo> getAllTasks() const;

signals:
	// 发送到工作线程的信号
	void addDownloadRequested(const DownloadTaskInfo& taskInfo);
	void pauseDownloadRequested(const QString& taskId);
	void resumeDownloadRequested(const QString& taskId);
	void cancelDownloadRequested(const QString& taskId);
	void speedLimitChanged(qint64 bytesPerSecond);
	void maxConcurrentChanged(int max);
	void maxThreadsChanged(int maxThreads);

	// 发送到UI线程的信号
	void downloadAdded(const QString& taskId);
	void downloadStarted(const QString& taskId);
	void downloadPaused(const QString& taskId);
	void downloadResumed(const QString& taskId);
	void downloadCanceled(const QString& taskId);
	void downloadCompleted(const QString& taskId, const QString& filePath);
	void downloadFailed(const QString& taskId, const QString& error);
	void downloadProgress(const QString& taskId, qint64 downloaded, qint64 total);

private slots:
	void onDownloadPaused(const QString& taskId);
	void onDownloadResumed(const QString& taskId);
	void onDownloadCanceled(const QString& taskId);
	void onDownloadCompleted(const QString& taskId, const QString& filePath);
	void onDownloadFailed(const QString& taskId, const QString& error);
	void onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total);

private:
	QThread m_workerThread;
	QMap<QString, DownloadTaskInfo> m_tasks;

	QString generateTaskId() const;
};