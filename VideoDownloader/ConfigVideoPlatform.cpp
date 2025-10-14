#include "ConfigVideoPlatform.h"
#include "ConfigManager.h"
#include <QUrlQuery>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>
#include <QRegularExpression>

ConfigVideoPlatform::ConfigVideoPlatform(const ModInfo& modInfo,
	QSharedPointer<INetworkManager> networkManager,
	QObject* parent)
	: QObject(parent)
	, m_modInfo(modInfo)
	, m_networkManager(networkManager)
{
}

bool ConfigVideoPlatform::matchesUrl(const QString& url) const
{
	for (const QString& pattern : m_modInfo.urlPatterns) {
		QRegularExpression regex(pattern);
		if (regex.match(url).hasMatch()) {
			return true;
		}
	}
	return false;
}

QFuture<VideoInfo> ConfigVideoPlatform::getVideoInfo(const QString& url)
{
	return QtConcurrent::run([this, url]() -> VideoInfo {
		try {
			LOG_INFO("ConfigVideoPlatform", "Getting video info for: %s", url.toUtf8().constData());

			QString videoId = extractVideoId(url);
			QString apiUrl = m_modInfo.getApiEndpoint("videoInfo");

			if (apiUrl.isEmpty()) {
				throw std::runtime_error("Video info API endpoint not configured");
			}

			// 构建请求参数
			QVariantMap params;
			QVariantMap headers = m_modInfo.getRequestHeaders();

			// 根据平台构建不同的参数
			if (m_modInfo.modId == "bilibili") {
				// B站API参数
				QRegularExpression bvRegex("BV[0-9A-Za-z]{10}");
				auto match = bvRegex.match(url);
				if (match.hasMatch()) {
					params["bvid"] = match.captured(0);
				}
			}
			else if (m_modInfo.modId == "youtube") {
				// YouTube API参数
				QRegularExpression youtubeRegex("(?:v=|/)([0-9A-Za-z_-]{11})");
				auto match = youtubeRegex.match(url);
				if (match.hasMatch()) {
					params["videoId"] = match.captured(1);
				}
			}

			// 发送请求
			NetworkResponse response;
			if (!params.isEmpty()) {
				QUrl fullUrl(apiUrl);
				QUrlQuery query;
				for (auto it = params.begin(); it != params.end(); ++it) {
					query.addQueryItem(it.key(), it.value().toString());
				}
				fullUrl.setQuery(query);
				response = m_networkManager->get(fullUrl.toString(), headers).result();
			}
			else {
				response = m_networkManager->get(apiUrl, headers).result();
			}

			if (!response.success) {
				throw std::runtime_error(response.errorString.toStdString());
			}

			QJsonDocument doc = QJsonDocument::fromJson(response.data);
			if (doc.isNull()) {
				throw std::runtime_error("Invalid JSON response");
			}

			VideoInfo videoInfo = parseVideoInfo(doc.object());
			videoInfo.platformId = m_modInfo.modId;

			LOG_INFO("ConfigVideoPlatform", "Video info retrieved: %s", videoInfo.title.toUtf8().constData());
			emit videoInfoReceived(videoInfo);

			return videoInfo;

		}
		catch (const std::exception& e) {
			LOG_ERROR("ConfigVideoPlatform", "Failed to get video info: %s", e.what());
			emit errorOccurred(QString("Failed to get video info: %1").arg(e.what()));
			throw;
		}
		});
}

QFuture<QList<StreamInfo>> ConfigVideoPlatform::getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
		try {
			LOG_INFO("ConfigVideoPlatform", "Getting video streams for: %s",
				videoInfo.title.toUtf8().constData());

			QString apiUrl = m_modInfo.getApiEndpoint("playUrl");
			if (apiUrl.isEmpty()) {
				throw std::runtime_error("Play URL API endpoint not configured");
			}

			// 构建请求参数
			QVariantMap params = buildRequestParams(videoInfo, request);
			QVariantMap headers = m_modInfo.getRequestHeaders();

			// 发送请求
			NetworkResponse response = m_networkManager->get(apiUrl, headers).result();
			if (!response.success) {
				throw std::runtime_error(response.errorString.toStdString());
			}

			QJsonDocument doc = QJsonDocument::fromJson(response.data);
			if (doc.isNull()) {
				throw std::runtime_error("Invalid JSON response");
			}

			QList<StreamInfo> streams = parseStreams(doc.object(), StreamType::Video);

			LOG_INFO("ConfigVideoPlatform", "Retrieved %d video streams", streams.size());
			emit streamsReceived(streams);

			return streams;

		}
		catch (const std::exception& e) {
			LOG_ERROR("ConfigVideoPlatform", "Failed to get video streams: %s", e.what());
			emit errorOccurred(QString("Failed to get video streams: %1").arg(e.what()));
			throw;
		}
		});
}

QFuture<QList<StreamInfo>> ConfigVideoPlatform::getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
		try {
			LOG_INFO("ConfigVideoPlatform", "Getting audio streams for: %s",
				videoInfo.title.toUtf8().constData());

			QString apiUrl = m_modInfo.getApiEndpoint("playUrl");
			if (apiUrl.isEmpty()) {
				throw std::runtime_error("Play URL API endpoint not configured");
			}

			// 构建请求参数
			QVariantMap params = buildRequestParams(videoInfo, request);
			QVariantMap headers = m_modInfo.getRequestHeaders();

			// 发送请求
			NetworkResponse response = m_networkManager->get(apiUrl, headers).result();
			if (!response.success) {
				throw std::runtime_error(response.errorString.toStdString());
			}

			QJsonDocument doc = QJsonDocument::fromJson(response.data);
			if (doc.isNull()) {
				throw std::runtime_error("Invalid JSON response");
			}

			QList<StreamInfo> streams = parseStreams(doc.object(), StreamType::Audio);

			LOG_INFO("ConfigVideoPlatform", "Retrieved %d audio streams", streams.size());
			emit streamsReceived(streams);

			return streams;

		}
		catch (const std::exception& e) {
			LOG_ERROR("ConfigVideoPlatform", "Failed to get audio streams: %s", e.what());
			emit errorOccurred(QString("Failed to get audio streams: %1").arg(e.what()));
			throw;
		}
		});
}

QFuture<SearchResult> ConfigVideoPlatform::searchVideos(const QString& keyword, int page)
{
	return QtConcurrent::run([this, keyword, page]() -> SearchResult {
		try {
			LOG_INFO("ConfigVideoPlatform", "Searching videos: %s", keyword.toUtf8().constData());

			QString apiUrl = m_modInfo.getApiEndpoint("search");
			if (apiUrl.isEmpty()) {
				throw std::runtime_error("Search API endpoint not configured");
			}

			// 构建请求参数
			QVariantMap params;
			QVariantMap headers = m_modInfo.getRequestHeaders();

			// 根据平台构建搜索参数
			if (m_modInfo.modId == "bilibili") {
				params["keyword"] = keyword;
				params["page"] = page;
				params["search_type"] = "video";
			}
			else if (m_modInfo.modId == "youtube") {
				params["q"] = keyword;
				params["page"] = page;
			}

			// 发送请求
			NetworkResponse response = m_networkManager->get(apiUrl, headers).result();
			if (!response.success) {
				throw std::runtime_error(response.errorString.toStdString());
			}

			QJsonDocument doc = QJsonDocument::fromJson(response.data);
			if (doc.isNull()) {
				throw std::runtime_error("Invalid JSON response");
			}

			// 解析搜索结果（简化实现）
			SearchResult result;
			result.searchQuery = keyword;
			result.platformId = m_modInfo.modId;

			// 这里需要根据具体平台的响应格式进行解析
			// 暂时返回空结果，实际实现时需要根据平台API文档实现

			LOG_INFO("ConfigVideoPlatform", "Search completed, found %d results", result.items.size());
			emit searchResultsReceived(result);

			return result;

		}
		catch (const std::exception& e) {
			LOG_ERROR("ConfigVideoPlatform", "Failed to search videos: %s", e.what());
			emit errorOccurred(QString("Failed to search videos: %1").arg(e.what()));
			throw;
		}
		});
}

VideoInfo ConfigVideoPlatform::parseVideoInfo(const QJsonObject& data)
{
	VideoInfo info;
	QVariantMap parserConfig = m_modInfo.getVideoInfoParser();

	// 使用配置的路径从JSON中提取数据
	info.title = extractJsonValue(data, parserConfig.value("title").toString()).toString();
	info.author = extractJsonValue(data, parserConfig.value("author").toString()).toString();
	info.description = extractJsonValue(data, parserConfig.value("description").toString()).toString();

	QVariant durationValue = extractJsonValue(data, parserConfig.value("duration").toString());
	if (durationValue.canConvert<double>()) {
		info.duration = durationValue.toDouble();
	}

	QString thumbnailUrl = extractJsonValue(data, parserConfig.value("thumbnail").toString()).toString();
	if (!thumbnailUrl.isEmpty()) {
		info.thumbnailUrl = QUrl(thumbnailUrl);
	}

	info.videoId = extractJsonValue(data, parserConfig.value("videoId").toString()).toString();

	return info;
}

QList<StreamInfo> ConfigVideoPlatform::parseStreams(const QJsonObject& data, StreamType type)
{
	QList<StreamInfo> streams;
	QVariantMap parserConfig = m_modInfo.getStreamParser(type);

	if (parserConfig.isEmpty()) {
		return streams;
	}

	QString arrayPath = parserConfig.value("arrayPath").toString();
	QJsonArray streamArray = extractJsonArray(data, arrayPath);

	if (streamArray.isEmpty()) {
		// 如果没有数组路径，尝试直接解析
		streamArray = QJsonArray({ data });
	}

	for (const QJsonValue& streamValue : streamArray) {
		QJsonObject streamObj = streamValue.toObject();

		StreamInfo stream;
		stream.id = extractJsonValue(streamObj, parserConfig.value("idPath").toString()).toString();
		stream.quality = extractJsonValue(streamObj, parserConfig.value("qualityPath").toString()).toString();

		QString urlStr = extractJsonValue(streamObj, parserConfig.value("urlPath").toString()).toString();
		if (!urlStr.isEmpty()) {
			stream.url = QUrl(urlStr);
		}

		QVariant bitrateValue = extractJsonValue(streamObj, parserConfig.value("bitratePath").toString());
		if (bitrateValue.canConvert<qint64>()) {
			stream.bitrate = bitrateValue.toLongLong();
		}

		if (type == StreamType::Video) {
			QVariant widthValue = extractJsonValue(streamObj, parserConfig.value("widthPath").toString());
			QVariant heightValue = extractJsonValue(streamObj, parserConfig.value("heightPath").toString());
			if (widthValue.canConvert<int>() && heightValue.canConvert<int>()) {
				stream.width = widthValue.toInt();
				stream.height = heightValue.toInt();
			}
		}

		if (stream.isValid()) {
			streams.append(stream);
		}
	}

	return streams;
}

QString ConfigVideoPlatform::extractVideoId(const QString& url)
{
	// 根据平台提取视频ID
	if (m_modInfo.modId == "bilibili") {
		QRegularExpression bvRegex("BV[0-9A-Za-z]{10}");
		auto match = bvRegex.match(url);
		if (match.hasMatch()) {
			return match.captured(0);
		}
	}
	else if (m_modInfo.modId == "youtube") {
		QRegularExpression youtubeRegex("(?:v=|/)([0-9A-Za-z_-]{11})");
		auto match = youtubeRegex.match(url);
		if (match.hasMatch()) {
			return match.captured(1);
		}
	}

	return QString();
}

QVariantMap ConfigVideoPlatform::buildRequestParams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	QVariantMap params;

	if (m_modInfo.modId == "bilibili") {
		params["bvid"] = videoInfo.videoId;
		QVariantMap qualityMapping = m_modInfo.getQualityMapping(request.type);
		params["qn"] = qualityMapping.value(request.quality);
	}
	else if (m_modInfo.modId == "youtube") {
		params["videoId"] = videoInfo.videoId;
		// YouTube 参数构建
	}

	return params;
}

QVariant ConfigVideoPlatform::extractJsonValue(const QJsonObject& data, const QString& path)
{
	if (path.isEmpty()) {
		return QVariant();
	}

	QStringList keys = path.split('.');
	QJsonValue current = data;

	for (const QString& key : keys) {
		if (current.isObject()) {
			current = current.toObject().value(key);
		}
		else if (current.isArray()) {
			bool ok;
			int index = key.toInt(&ok);
			if (ok && index >= 0 && index < current.toArray().size()) {
				current = current.toArray().at(index);
			}
			else {
				return QVariant();
			}
		}
		else {
			return QVariant();
		}

		if (current.isUndefined()) {
			return QVariant();
		}
	}

	return current.toVariant();
}

QJsonArray ConfigVideoPlatform::extractJsonArray(const QJsonObject& data, const QString& path)
{
	QVariant value = extractJsonValue(data, path);
	if (value.canConvert<QJsonArray>()) {
		return value.toJsonArray();
	}
	return QJsonArray();
}