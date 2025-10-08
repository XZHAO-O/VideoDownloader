#pragma once

#include <QMetaType>

// 应用程序状态枚举
enum ApplicationState {
	Uninitialized = 0,
	Initializing,
	Running,
	ShuttingDown,
	Error
};

// 下载任务状态
enum DownloadStatus {
	Queued = 0,
	Downloading,
	Paused,
	Completed,
	Failed,
	Cancelled
};

// 视频质量选项
struct VideoQuality {
	QString id;
	QString name;
	int width;
	int height;
	qint64 bitrate;

	bool operator==(const VideoQuality& other) const {
		return id == other.id;
	}
};

// 音频质量选项
struct AudioQuality {
	QString id;
	QString name;
	qint64 bitrate;
	QString codec;

	bool operator==(const AudioQuality& other) const {
		return id == other.id;
	}
};

// 下载格式
enum DownloadFormat {
	VideoOnly = 0,
	AudioOnly,
	VideoAudio,
	Merged
};

Q_DECLARE_METATYPE(ApplicationState)
Q_DECLARE_METATYPE(DownloadStatus)
Q_DECLARE_METATYPE(VideoQuality)
Q_DECLARE_METATYPE(AudioQuality)
Q_DECLARE_METATYPE(DownloadFormat)