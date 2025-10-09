#pragma once

#include "IMod.h"
#include "VideoInfo.h"
#include "SearchResult.h"
#include "VideoDownloadRequest.h"
#include <QFuture>

class IVideoPlatformMod : public IMod
{
	Q_OBJECT

public:
	explicit IVideoPlatformMod(QObject* parent = nullptr) : IMod(parent) {}
	virtual ~IVideoPlatformMod() = default;

	// 平台基本信息
	virtual QString platformId() const = 0;
	virtual QString platformName() const = 0;
	virtual QString platformIcon() const = 0;
	virtual QList<QString> supportedDomains() const = 0;

	// 视频信息获取
	virtual QFuture<VideoInfo> getVideoInfo(const QUrl& videoUrl) = 0;
	virtual QFuture<QList<StreamInfo>> getVideoStreams(const QString& videoId,
		const VideoQuality& quality) = 0;
	virtual QFuture<QList<StreamInfo>> getAudioStreams(const QString& videoId,
		const AudioQuality& quality) = 0;

	// 搜索功能
	virtual QFuture<SearchResult> searchVideos(const QString& query,
		int page = 1,
		int resultsPerPage = 20) = 0;
	virtual QFuture<SearchResult> searchVideosByChannel(const QString& channelId,
		int page = 1,
		int resultsPerPage = 20) = 0;

	// 下载相关
	virtual QFuture<bool> downloadVideo(const VideoDownloadRequest& request) = 0;
	virtual bool supportsFormat(DownloadFormat format) const = 0;
	virtual QList<VideoQuality> getSupportedVideoQualities() const = 0;
	virtual QList<AudioQuality> getSupportedAudioQualities() const = 0;

	// 验证和检查
	virtual bool isValidUrl(const QUrl& url) const = 0;
	virtual bool isAvailable() const = 0;
	virtual QString getLastError() const = 0;

	// 高级功能
	virtual QFuture<QList<VideoInfo>> getChannelVideos(const QString& channelId,
		int page = 1,
		int resultsPerPage = 20) = 0;
	virtual QFuture<QList<VideoInfo>> getPlaylistVideos(const QString& playlistId,
		int page = 1,
		int resultsPerPage = 20) = 0;

signals:
	void videoInfoReceived(const VideoInfo& videoInfo);
	void searchResultsReceived(const SearchResult& results);
	void downloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void downloadCompleted(const QString& taskId, const QString& filePath);
	void downloadFailed(const QString& taskId, const QString& error);
};

Q_DECLARE_INTERFACE(IVideoPlatformMod, "com.videodownloader.IVideoPlatformMod/1.0")