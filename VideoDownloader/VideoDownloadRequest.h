#pragma once

#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QRandomGenerator>
#include "ApplicationState.h"

struct VideoDownloadRequest {
	QString taskId;
	QUrl videoUrl;
	QString platformId;
	QString outputPath;
	VideoQuality videoStream;
	AudioQuality audioStream;
	DownloadFormat format;
	QDateTime requestTime;
	int retryCount = 0;
	int maxRetries = 3;

	// 验证请求有效性
	bool isValid() const {
		return !taskId.isEmpty() &&
			videoUrl.isValid() &&
			!platformId.isEmpty() &&
			!outputPath.isEmpty();
	}

	// 生成唯一任务ID
	static QString generateTaskId() {
		return QString("task_%1_%2")
			.arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
			.arg(QRandomGenerator::global()->generate() % 10000);
	}
};

Q_DECLARE_METATYPE(VideoDownloadRequest)