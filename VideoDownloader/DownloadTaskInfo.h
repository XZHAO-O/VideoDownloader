#pragma once

#include "VideoDownloadRequest.h"
#include "ModInfo.h"

struct DownloadTaskInfo {
	QString taskId;
	VideoDownloadRequest request;
	StreamRequest streamRequest;
	DownloadStatus status;
	qint64 downloadedBytes;
	qint64 totalBytes;
	double downloadSpeed; // bytes per second
	QDateTime startTime;
	QDateTime endTime;
	QString currentFile;
	QString errorMessage;
	int progressPercentage;

	DownloadTaskInfo()
		: status(Queued)
		, downloadedBytes(0)
		, totalBytes(0)
		, downloadSpeed(0)
		, progressPercentage(0)
	{
	}

	bool operator==(const DownloadTaskInfo& other) const {
		return taskId == other.taskId;
	}

	// 更新进度
	void updateProgress(qint64 downloaded, qint64 total) {
		downloadedBytes = downloaded;
		totalBytes = total;
		downloadSpeed = calculateSpeed();
		progressPercentage = total > 0 ? static_cast<int>((downloaded * 100) / total) : 0;
	}

	// 计算下载速度（需要定期调用）
	double calculateSpeed() const {
		if (startTime.isNull()) return 0;

		qint64 elapsed = startTime.msecsTo(QDateTime::currentDateTime());
		if (elapsed <= 0) return 0;

		return (downloadedBytes * 1000.0) / elapsed; // bytes per second
	}

	// 估计剩余时间
	QString estimatedTimeRemaining() const {
		if (downloadSpeed <= 0 || totalBytes <= downloadedBytes)
			return "未知";

		qint64 remainingBytes = totalBytes - downloadedBytes;
		int seconds = static_cast<int>(remainingBytes / downloadSpeed);

		if (seconds < 60)
			return QString("%1秒").arg(seconds);
		else if (seconds < 3600)
			return QString("%1分%2秒").arg(seconds / 60).arg(seconds % 60);
		else
			return QString("%1时%2分").arg(seconds / 3600).arg((seconds % 3600) / 60);
	}

	// 格式化文件大小
	static QString formatFileSize(qint64 bytes) {
		const qint64 KB = 1024;
		const qint64 MB = KB * 1024;
		const qint64 GB = MB * 1024;

		if (bytes >= GB)
			return QString("%1 GB").arg(QString::number(bytes / static_cast<double>(GB), 'f', 2));
		else if (bytes >= MB)
			return QString("%1 MB").arg(QString::number(bytes / static_cast<double>(MB), 'f', 2));
		else if (bytes >= KB)
			return QString("%1 KB").arg(bytes / KB);
		else
			return QString("%1 字节").arg(bytes);
	}
};

Q_DECLARE_METATYPE(DownloadTaskInfo)