#pragma once

#include <QVariantMap>
#include <QByteArray>

class ConfigVideoPlatform;

enum class StreamType
{
	AVMerged = 0,
	AVSeparate
};

// 视频流信息
struct StreamInfo
{
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
struct VideoInfo
{
	QString videoId;
	QString title;
	QString sectionName;
	QString author;
	QString duration;
	QString publishTime;
	QUrl coverUrl;
	QByteArray cover;
	StreamType streamType;
	QSharedPointer<ConfigVideoPlatform> videoPlatform;

	// 额外参数存储（如aid, cid等）
	QVariantMap extraParams;

	bool isValid() const
	{
		return !videoId.isEmpty() && !title.isEmpty();
	}
};

Q_DECLARE_METATYPE(VideoInfo)
Q_DECLARE_METATYPE(StreamInfo)