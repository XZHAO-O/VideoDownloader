// PlatformAggregatorService.cpp
#include "PlatformAggregatorService.h"
#include <QtConcurrent\QtConcurrent>

PlatformAggregatorService::PlatformAggregatorService(QSharedPointer<ModManager> modManager,
	QObject* parent)
	: QObject(parent)
	, m_modManager(modManager)
{
}

QList<QString> PlatformAggregatorService::getAvailablePlatforms() const
{
	return m_modManager ? m_modManager->getVideoPlatformMods() : QList<QString>();
}

QSharedPointer<IVideoPlatformMod> PlatformAggregatorService::getPlatform(const QString& platformId) const
{
	return m_modManager ? m_modManager->getVideoPlatformMod(platformId) : nullptr;
}

QSharedPointer<IVideoPlatformMod> PlatformAggregatorService::getPlatformForUrl(const QUrl& url) const
{
	return m_modManager ? m_modManager->getVideoPlatformModForUrl(url) : nullptr;
}

QFuture<VideoInfo> PlatformAggregatorService::getVideoInfo(const QUrl& videoUrl)
{
	return QtConcurrent::run([this, videoUrl]() -> VideoInfo {
		auto platform = getPlatformForUrl(videoUrl);
		if (!platform) {
			LogSystem::instance().error(
				QString("No platform found for URL: %1").arg(videoUrl.toString()),
				"PlatformAggregator");
			return VideoInfo();
		}

		try {
			auto future = platform->getVideoInfo(videoUrl);
			future.waitForFinished();
			return future.result();
		}
		catch (const std::exception& e) {
			LogSystem::instance().error(
				QString("Failed to get video info: %1").arg(e.what()),
				"PlatformAggregator");
			return VideoInfo();
		}
		});
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
			auto future = platform->getVideoStreams(videoId, quality);
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
			auto future = platform->getAudioStreams(videoId, quality);
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

QFuture<SearchResult> PlatformAggregatorService::searchVideos(const QString& query,
	const QString& platformId,
	int page, int resultsPerPage)
{
	return QtConcurrent::run([this, query, platformId, page, resultsPerPage]() -> SearchResult {
		if (platformId.isEmpty()) {
			// 在所有平台上搜索（简化实现，只搜索第一个平台）
			auto platforms = getAvailablePlatforms();
			if (platforms.isEmpty()) {
				return SearchResult();
			}
			auto platform = getPlatform(platforms.first());
			if (platform) {
				auto future = platform->searchVideos(query, page, resultsPerPage);
				future.waitForFinished();
				return future.result();
			}
		}
		else {
			auto platform = getPlatform(platformId);
			if (platform) {
				auto future = platform->searchVideos(query, page, resultsPerPage);
				future.waitForFinished();
				return future.result();
			}
		}
		return SearchResult();
		});
}

QFuture<SearchResult> PlatformAggregatorService::searchVideosByChannel(const QString& channelId,
	const QString& platformId,
	int page, int resultsPerPage)
{
	return QtConcurrent::run([this, channelId, platformId, page, resultsPerPage]() -> SearchResult {
		auto platform = getPlatform(platformId);
		if (!platform) {
			return SearchResult();
		}

		try {
			auto future = platform->searchVideosByChannel(channelId, page, resultsPerPage);
			future.waitForFinished();
			return future.result();
		}
		catch (const std::exception& e) {
			LogSystem::instance().error(
				QString("Failed to search channel videos: %1").arg(e.what()),
				"PlatformAggregator");
			return SearchResult();
		}
		});
}

QFuture<QList<VideoInfo>> PlatformAggregatorService::getBatchVideoInfo(const QList<QUrl>& videoUrls)
{
	return QtConcurrent::run([this, videoUrls]() -> QList<VideoInfo> {
		QList<QFuture<VideoInfo>> futures;
		for (const auto& url : videoUrls) {
			futures.append(getVideoInfo(url));
		}

		QList<VideoInfo> results;
		for (auto& future : futures) {
			future.waitForFinished();
			VideoInfo info = future.result();
			if (info.isValid()) {
				results.append(info);
			}
		}

		return results;
		});
}