// PlatformAggregatorService.h
#pragma once

#include <QObject>
#include <QSharedPointer>
#include "ModManager.h"
#include "VideoInfo.h"
#include "SearchResult.h"

class PlatformAggregatorService : public QObject
{
	Q_OBJECT

public:
	explicit PlatformAggregatorService(QSharedPointer<ModManager> modManager,
		QObject* parent = nullptr);

	// 平台查询
	QList<QString> getAvailablePlatforms() const;
	QSharedPointer<IVideoPlatformMod> getPlatform(const QString& platformId) const;
	QSharedPointer<IVideoPlatformMod> getPlatformForUrl(const QUrl& url) const;

	// 视频信息获取
	QFuture<VideoInfo> getVideoInfo(const QUrl& videoUrl);
	QFuture<QList<StreamInfo>> getVideoStreams(const QString& videoId, const QString& platformId,
		const VideoQuality& quality);
	QFuture<QList<StreamInfo>> getAudioStreams(const QString& videoId, const QString& platformId,
		const AudioQuality& quality);

	// 搜索功能
	QFuture<SearchResult> searchVideos(const QString& query, const QString& platformId = "",
		int page = 1, int resultsPerPage = 20);
	QFuture<SearchResult> searchVideosByChannel(const QString& channelId, const QString& platformId,
		int page = 1, int resultsPerPage = 20);

	// 批量操作
	QFuture<QList<VideoInfo>> getBatchVideoInfo(const QList<QUrl>& videoUrls);

private:
	QSharedPointer<ModManager> m_modManager;
};