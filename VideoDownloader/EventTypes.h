#pragma once

#include <QDateTime>

// 下载开始事件
struct DownloadStartedEvent {
	QString taskId;
	QString videoTitle;
	QDateTime startTime;
};

// 下载进度事件
struct DownloadProgressEvent {
	QString taskId;
	qint64 downloadedBytes;
	qint64 totalBytes;
	double speed; // bytes per second
};

// 下载完成事件
struct DownloadCompletedEvent {
	QString taskId;
	QString filePath;
	bool success;
	QString errorMessage;
};

// 注册元类型，以便在信号槽中使用
Q_DECLARE_METATYPE(DownloadStartedEvent)
Q_DECLARE_METATYPE(DownloadProgressEvent)
Q_DECLARE_METATYPE(DownloadCompletedEvent)