#include "PlatformAggregatorService.h"

#include <QtConcurrent\QtConcurrent>

#include "ModInfo.h"
#include "LogSystem.h"
#include "ConfigModManager.h"
#include "ConfigVideoPlatform.h"
#include "AntMessageManager.h"

PlatformAggregatorService::PlatformAggregatorService(QSharedPointer<ConfigModManager> modManager,
	QObject* parent)
	: QObject(parent)
	, m_modManager(modManager)
{
}

QList<QString> PlatformAggregatorService::getAvailablePlatforms() const
{
	return m_modManager ? m_modManager->getAvailablePlatforms() : QList<QString>();
}

QSharedPointer<ConfigVideoPlatform> PlatformAggregatorService::getPlatform(const QString& platformId) const
{
	return m_modManager ? m_modManager->getPlatformForMod(platformId) : nullptr;
}

QSharedPointer<ConfigVideoPlatform> PlatformAggregatorService::getPlatformForUrl(const QUrl& url) const
{
	return m_modManager ? m_modManager->getPlatformForUrl(url.toString()) : nullptr;
}

QList<VideoInfo> PlatformAggregatorService::getVideoInfo(const QUrl& videoUrl)
{
	auto platform = getPlatformForUrl(videoUrl);
	if (!platform)
	{
		LOG_WARN("PlatformAggregator", QString("No platform found for URL: %1").arg(videoUrl.toString()));
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, "无法解析视频链接");
		return QList<VideoInfo>();
	}

	return platform->getVideoInfo(videoUrl.toString());
}

QFuture<QList<StreamInfo>> PlatformAggregatorService::getVideoStreams(const QString& videoId,
	const QString& platformId,
	const VideoQuality& quality)
{
	return QtConcurrent::run([this, videoId, platformId, quality]() -> QList<StreamInfo> {
		auto platform = getPlatform(platformId);
		if (!platform) {
			LogSystem::instance().error(
				QString("Platform not found: %1").arg(platformId),
				"PlatformAggregator");
			return QList<StreamInfo>();
		}

		try {
			// 创建StreamRequest - 使用正确的结构
			StreamRequest request;
			request.quality = quality.id;
			//request.type = StreamType::Video;

			// 需要先获取VideoInfo
			VideoInfo videoInfo;
			videoInfo.videoId = videoId;
			//videoInfo.platformId = platformId;

			auto future = platform->getVideoStreams(videoInfo, request);
			future.waitForFinished();
			return future.result();
		}
		catch (const std::exception& e) {
			LogSystem::instance().error(
				QString("Failed to get video streams: %1").arg(e.what()),
				"PlatformAggregator");
			return QList<StreamInfo>();
		}
		});
}

QFuture<QList<StreamInfo>> PlatformAggregatorService::getAudioStreams(const QString& videoId,
	const QString& platformId,
	const AudioQuality& quality)
{
	return QtConcurrent::run([this, videoId, platformId, quality]() -> QList<StreamInfo> {
		auto platform = getPlatform(platformId);
		if (!platform) {
			LogSystem::instance().error(
				QString("Platform not found: %1").arg(platformId),
				"PlatformAggregator");
			return QList<StreamInfo>();
		}

		try {
			// 创建StreamRequest - 使用正确的结构
			StreamRequest request;
			request.quality = quality.id;
			//request.type = StreamType::Audio;

			// 需要先获取VideoInfo
			VideoInfo videoInfo;
			videoInfo.videoId = videoId;
			//videoInfo.platformId = platformId;

			auto future = platform->getAudioStreams(videoInfo, request);
			future.waitForFinished();
			return future.result();
		}
		catch (const std::exception& e) {
			LogSystem::instance().error(
				QString("Failed to get audio streams: %1").arg(e.what()),
				"PlatformAggregator");
			return QList<StreamInfo>();
		}
		});
}

//QFuture<SearchResult> PlatformAggregatorService::searchVideos(const QString& query,
//	const QString& platformId,
//	int page, int resultsPerPage)
//{
//	return QtConcurrent::run([this, query, platformId, page, resultsPerPage]() -> SearchResult {
//		if (platformId.isEmpty()) {
//			// 在所有平台上搜索
//			auto platforms = getAvailablePlatforms();
//			if (platforms.isEmpty()) {
//				return SearchResult();
//			}
//
//			// 尝试使用第一个平台进行搜索
//			auto platform = getPlatform(platforms.first());
//			if (platform) {
//				auto future = platform->searchVideos(query, page);
//				future.waitForFinished();
//				return future.result();
//			}
//		}
//		else {
//			auto platform = getPlatform(platformId);
//			if (platform) {
//				auto future = platform->searchVideos(query, page);
//				future.waitForFinished();
//				return future.result();
//			}
//		}
//		return SearchResult();
//		});
//}

//QFuture<SearchResult> PlatformAggregatorService::searchVideosByChannel(const QString& channelId,
//	const QString& platformId,
//	int page, int resultsPerPage)
//{
//	return QtConcurrent::run([this, channelId, platformId, page, resultsPerPage]() -> SearchResult {
//		auto platform = getPlatform(platformId);
//		if (!platform) {
//			return SearchResult();
//		}
//
//		try {
//			// 注意：ConfigVideoPlatform目前没有searchVideosByChannel方法
//			// 暂时使用普通搜索
//			auto future = platform->searchVideos(channelId, page);
//			future.waitForFinished();
//			return future.result();
//		}
//		catch (const std::exception& e) {
//			LogSystem::instance().error(
//				QString("Failed to search channel videos: %1").arg(e.what()),
//				"PlatformAggregator");
//			return SearchResult();
//		}
//		});
//}

QFuture<QList<VideoInfo>> PlatformAggregatorService::getBatchVideoInfo(const QList<QUrl>& videoUrls)
{
	return QtConcurrent::run([this, videoUrls]() -> QList<VideoInfo> {
		QList<VideoInfo> videoInfos;
		for (const auto& url : videoUrls) {
			videoInfos.append(getVideoInfo(url));
		}

		return videoInfos;
		});
}