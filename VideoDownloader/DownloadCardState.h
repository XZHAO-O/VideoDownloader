#pragma once

#include <QMetaType>

// 下载卡片状态枚举
enum class DownloadCardState {
	Pending,        // 待下载
	Downloading,    // 下载中
	Downloaded,     // 已下载
	Error           // 错误
};

// 音质等级
enum class AudioQualityLevel {
	Low = 0,
	Medium,
	High,
	Ultra
};

// 画质等级  
enum class VideoQualityLevel {
	Low = 0,    // 480p
	Medium,     // 720p
	High,       // 1080p
	Ultra,      // 4K
	Original    // 8K
};

Q_DECLARE_METATYPE(DownloadCardState)
Q_DECLARE_METATYPE(AudioQualityLevel)
Q_DECLARE_METATYPE(VideoQualityLevel)