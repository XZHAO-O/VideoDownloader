#pragma once

#include <QUrl>
#include <QVariantMap>

#include "ApplicationState.h"

class IDownloadEngine : public QObject
{
	Q_OBJECT

public:
	// 添加构造函数声明
	explicit IDownloadEngine(QObject* parent = nullptr) : QObject(parent) {}

	struct DownloadTask {
		QString taskId;
		QUrl url;  // 修改为 QUrl 类型
		QString savePath;
		QVariantMap headers;
		qint64 startByte = 0;
		qint64 endByte = -1; // -1 表示到文件末尾
	};

	virtual ~IDownloadEngine() = default;

	// 任务管理
	virtual QString addTask(const DownloadTask& task) = 0;
	virtual bool removeTask(const QString& taskId) = 0;
	virtual bool pauseTask(const QString& taskId) = 0;
	virtual bool resumeTask(const QString& taskId) = 0;
	virtual bool cancelTask(const QString& taskId) = 0;

	// 任务状态
	virtual QList<QString> getActiveTasks() const = 0;
	virtual bool isTaskActive(const QString& taskId) const = 0;
	virtual qint64 getTaskProgress(const QString& taskId) const = 0;
	virtual qint64 getTaskTotalSize(const QString& taskId) const = 0;

	// 引擎控制
	virtual void setMaxConcurrentDownloads(int count) = 0;
	virtual int maxConcurrentDownloads() const = 0;
	virtual void setDownloadSpeedLimit(qint64 bytesPerSecond) = 0;
	virtual qint64 downloadSpeedLimit() const = 0;

signals:
	void taskProgressChanged(const QString& taskId, qint64 downloaded, qint64 total);
	void taskStatusChanged(const QString& taskId, DownloadStatus status);
	void taskCompleted(const QString& taskId, const QString& filePath);
	void taskFailed(const QString& taskId, const QString& error);
	void downloadSpeedUpdated(qint64 bytesPerSecond);
};