#pragma once

#include <QUrl>
#include <QDateTime>
#include <QUuid>
#include <chrono>
#include <QRandomGenerator>

#include "ApplicationState.h"

struct VideoDownloadRequest {
	QString taskId;
	QUrl videoPlayUrl;
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
			videoPlayUrl.isValid() &&
			!platformId.isEmpty() &&
			!outputPath.isEmpty();
	}

	// 生成唯一任务ID
	static QString generateTaskId()
	{
		using namespace std::chrono;

		// 微秒级时间戳（性能与精度的平衡）
		auto now = high_resolution_clock::now();
		auto micros = duration_cast<microseconds>(now.time_since_epoch()).count();

		// 紧凑型UUID（移除分隔符和连字符）
		QString uuid = QUuid::createUuid().toString(QUuid::Id128);

		// 额外随机数
		uint32_t randomNum = QRandomGenerator::global()->generate();

		return QString("task_%1_%2_%3")
			.arg(micros)
			.arg(uuid)
			.arg(randomNum, 8, 16, QChar('0'));
	}
};

Q_DECLARE_METATYPE(VideoDownloadRequest)