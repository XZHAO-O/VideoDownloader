#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QFile>
#include "DownloadTaskInfo.h"

class DownloadItem;

class DownloadEngine : public QObject
{
	Q_OBJECT

public:
	explicit DownloadEngine(QObject* parent = nullptr);
	~DownloadEngine();

public slots:
	void onAddDownload(const DownloadTaskInfo& taskInfo);
	void onPauseDownload(const QString& taskId);
	void onResumeDownload(const QString& taskId);
	void onCancelDownload(const QString& taskId);
	void onSpeedLimitChanged(qint64 bytesPerSecond);
	void onMaxConcurrentChanged(int max);
	void onMaxThreadsChanged(int maxThreads);

private slots:
	void onDownloadItemFinished(const QString& taskId);

signals:
	void downloadAdded(const QString& taskId);
	void downloadStarted(const QString& taskId);
	void downloadPaused(const QString& taskId);
	void downloadResumed(const QString& taskId);
	void downloadCanceled(const QString& taskId);
	void downloadCompleted(const QString& taskId, const QString& filePath);
	void downloadFailed(const QString& taskId, const QString& error);
	void downloadProgress(const QString& taskId, qint64 downloaded, qint64 total);

private:
	void processQueue();
	void startNextDownload(const DownloadTaskInfo& taskInfo);
	void cleanupDownload(const QString& taskId);

	QNetworkAccessManager* m_networkManager;

	QQueue<DownloadTaskInfo> m_downloadQueue;
	QMap<QString, DownloadItem*> m_activeDownloads;
	QMap<QString, DownloadTaskInfo> m_allTasks;

	QMutex m_queueMutex;

	int m_maxConcurrentDownloads;
	int m_currentDownloads;
	qint64 m_downloadSpeedLimit;
	int m_maxThreadsPerDownload;
};

class DownloadItem : public QObject
{
	Q_OBJECT

public:
	DownloadItem(const DownloadTaskInfo& taskInfo, QNetworkAccessManager* manager, QObject* parent = nullptr);
	~DownloadItem();

	void start();
	void pause();
	void resume();
	void cancel();

	QString taskId() const { return m_taskInfo.taskId; }
	DownloadStatus status() const { return m_taskInfo.status; }

signals:
	void finished(const QString& taskId);
	void progress(const QString& taskId, qint64 downloaded, qint64 total);

private slots:
	void onReadyRead();
	void onFinished();
	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onErrorOccurred(QNetworkReply::NetworkError error);

private:
	void cleanup();

	DownloadTaskInfo m_taskInfo;
	QNetworkAccessManager* m_networkManager;
	QNetworkReply* m_reply;
	QFile* m_file;

	qint64 m_downloadedBytes;
	qint64 m_totalBytes;

	bool m_isPaused;
	bool m_isCanceled;
};