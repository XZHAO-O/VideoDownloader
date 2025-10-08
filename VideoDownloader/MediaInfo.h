#pragma once

#include <QString>
#include <QSize>
#include <QJsonObject>
#include <QJsonArray>

// 媒体文件信息
struct MediaInfo {
	QString filePath;
	QString format;
	qint64 fileSize;
	double duration; // seconds
	double bitrate; // bits per second

	// 视频流信息
	struct VideoStream {
		QString codec;
		QSize resolution;
		double frameRate;
		qint64 bitrate;
		QString pixelFormat;
		bool isValid() const { return !codec.isEmpty(); }
	} videoStream;

	// 音频流信息
	struct AudioStream {
		QString codec;
		int sampleRate;
		int channels;
		qint64 bitrate;
		bool isValid() const { return !codec.isEmpty(); }
	} audioStream;

	bool isValid() const {
		return !filePath.isEmpty() && fileSize > 0;
	}

	// 从 FFprobe JSON 解析
	static MediaInfo fromFFprobeJson(const QJsonObject& ffprobeOutput) {
		MediaInfo info;

		if (ffprobeOutput.contains("format")) {
			QJsonObject format = ffprobeOutput["format"].toObject();
			info.filePath = format["filename"].toString();
			info.format = format["format_name"].toString();
			info.fileSize = format["size"].toString().toLongLong();
			info.duration = format["duration"].toString().toDouble();
			info.bitrate = format["bit_rate"].toString().toLongLong();
		}

		if (ffprobeOutput.contains("streams")) {
			QJsonArray streams = ffprobeOutput["streams"].toArray();
			for (const auto& streamValue : streams) {
				QJsonObject stream = streamValue.toObject();
				QString codecType = stream["codec_type"].toString();

				if (codecType == "video" && !info.videoStream.isValid()) {
					info.videoStream.codec = stream["codec_name"].toString();
					info.videoStream.resolution = QSize(
						stream["width"].toInt(),
						stream["height"].toInt()
					);
					info.videoStream.frameRate = parseFrameRate(stream["r_frame_rate"].toString());
					info.videoStream.bitrate = stream["bit_rate"].toString().toLongLong();
					info.videoStream.pixelFormat = stream["pix_fmt"].toString();
				}
				else if (codecType == "audio" && !info.audioStream.isValid()) {
					info.audioStream.codec = stream["codec_name"].toString();
					info.audioStream.sampleRate = stream["sample_rate"].toString().toInt();
					info.audioStream.channels = stream["channels"].toInt();
					info.audioStream.bitrate = stream["bit_rate"].toString().toLongLong();
				}
			}
		}

		return info;
	}

private:
	static double parseFrameRate(const QString& frameRateStr) {
		if (frameRateStr.contains('/')) {
			QStringList parts = frameRateStr.split('/');
			if (parts.size() == 2) {
				double num = parts[0].toDouble();
				double den = parts[1].toDouble();
				return den != 0 ? num / den : 0;
			}
		}
		return frameRateStr.toDouble();
	}
};

Q_DECLARE_METATYPE(MediaInfo)
Q_DECLARE_METATYPE(MediaInfo::VideoStream)
Q_DECLARE_METATYPE(MediaInfo::AudioStream)