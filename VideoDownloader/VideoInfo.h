#pragma once

#include <QVariantMap>

// 视频流信息
struct StreamInfo {
	QString id;
	QUrl url;
	QString quality;
	QString codec;
	qint64 bitrate;
	qint64 fileSize;
	int width;
	int height;
	double duration; // seconds
	QVariantMap headers; // 请求头信息

	bool isValid() const {
		return !id.isEmpty() && url.isValid();
	}
};

// 视频基本信息
struct VideoInfo {
	QString videoId;
	QString title;
	QString author;
	QString description;
	QUrl thumbnailUrl;
	QString duration; // seconds
	QDateTime uploadDate;
	qint64 viewCount;
	qint64 likeCount;
	QString category;
	QList<QString> tags;
	QString platformId;

	// 额外参数存储（如aid, cid等）
	QVariantMap extraParams;

	// 可用流
	QList<StreamInfo> videoStreams;
	QList<StreamInfo> audioStreams;

	bool isValid() const {
		return !videoId.isEmpty() && !title.isEmpty();
	}

	// 格式化时长
	QString formattedDuration(int totalSeconds) const {
		int hours = totalSeconds / 3600;
		int minutes = (totalSeconds % 3600) / 60;
		int seconds = totalSeconds % 60;

		if (hours > 0)
			return QString("%1:%2:%3")
			.arg(hours, 2, 10, QLatin1Char('0'))
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(seconds, 2, 10, QLatin1Char('0'));
		else
			return QString("%1:%2")
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(seconds, 2, 10, QLatin1Char('0'));
	}

	// 格式化观看数
	QString formattedViewCount() const {
		if (viewCount >= 1000000)
			return QString("%1M").arg(QString::number(viewCount / 1000000.0, 'f', 1));
		else if (viewCount >= 1000)
			return QString("%1K").arg(viewCount / 1000);
		else
			return QString::number(viewCount);
	}
};

Q_DECLARE_METATYPE(VideoInfo)
Q_DECLARE_METATYPE(StreamInfo)