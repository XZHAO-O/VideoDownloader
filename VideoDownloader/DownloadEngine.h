#pragma once

#include "IDownloadEngine.h"

#include <QRunnable>
#include <QQueue>
#include <QDateTime>
#include <QMutex>

#include "ApplicationState.h"

class QFile;
class QThreadPool;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class LogSystem;
class ConfigManager;

class DownloadEngine : public IDownloadEngine
{
	Q_OBJECT

public:
	explicit DownloadEngine(QSharedPointer<ConfigManager> configManager,
		QObject* parent = nullptr);
	~DownloadEngine();

	// IDownloadEngine 接口实现
	QString addTask(const DownloadTask& task) override;
	bool removeTask(const QString& taskId) override;
	bool pauseTask(const QString& taskId) override;
	bool resumeTask(const QString& taskId) override;
	bool cancelTask(const QString& taskId) override;

	QList<QString> getActiveTasks() const override;
	bool isTaskActive(const QString& taskId) const override;
	qint64 getTaskProgress(const QString& taskId) const override;
	qint64 getTaskTotalSize(const QString& taskId) const override;

	void setMaxConcurrentDownloads(int count) override;
	int maxConcurrentDownloads() const override;
	void setDownloadSpeedLimit(qint64 bytesPerSecond) override;
	qint64 downloadSpeedLimit() const override;

private slots:
	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onDownloadFinished();
	void onReadyRead();
	void processTaskQueue();
	void updateSpeedStatistics();

private:
	struct DownloadTaskItem {
		DownloadTask task;
		QNetworkReply* reply = nullptr;
		QFile* file = nullptr;
		qint64 downloadedBytes = 0;
		qint64 totalBytes = 0;
		DownloadStatus status = Queued;
		QDateTime startTime;
		QDateTime lastUpdateTime;
		qint64 lastDownloadedBytes = 0;
		int retryCount = 0;
		int maxRetries = 3;
		QString errorString;

		bool isValid() const {
			return !task.taskId.isEmpty() &&
				task.url.isValid() &&
				!task.savePath.isEmpty();
		}
	};

	class DownloadRunnable : public QRunnable {
	public:
		DownloadRunnable(DownloadEngine* engine, const QString& taskId);
		void run() override;

	private:
		DownloadEngine* m_engine;
		QString m_taskId;
	};

	bool startDownload(const QString& taskId);
	bool stopDownload(const QString& taskId);
	void cleanupTask(const QString& taskId);
	void retryTask(const QString& taskId);
	void completeTask(const QString& taskId, bool success);
	void updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total);
	qint64 calculateCurrentSpeed(const DownloadTaskItem& task) const;

	QSharedPointer<ConfigManager> m_configManager;
	LogSystem* m_logger;

	QNetworkAccessManager* m_networkManager;
	QThreadPool* m_threadPool;
	QTimer* m_queueTimer;
	QTimer* m_speedTimer;

	mutable QMutex m_tasksMutex;
	QMap<QString, DownloadTaskItem> m_tasks;
	QQueue<QString> m_taskQueue;

	int m_maxConcurrentDownloads = 3;
	qint64 m_downloadSpeedLimit = 0; // 0 means no limit
	qint64 m_currentTotalSpeed = 0;
	QMap<QString, qint64> m_taskSpeeds;
};