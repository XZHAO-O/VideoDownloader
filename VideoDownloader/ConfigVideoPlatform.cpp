#include "ConfigVideoPlatform.h"

#include <QtConcurrent/QtConcurrent>

#include "LogSystem.h"
#include "ConfigManager.h"
#include "INetworkManager.h"
#include "DownloadTaskInfo.h"
#include "AntMessageManager.h"

ConfigVideoPlatform::ConfigVideoPlatform(const ModInfo& modInfo, QString modPath, QSharedPointer<INetworkManager> networkManager,
	QObject* parent)
	: QObject(parent)
	, m_modInfo(modInfo)
	, m_networkManager(networkManager)
{
	// 初始化登录管理器
	m_loginManager.reset(new LoginManager(modInfo, modPath, networkManager, this));

	// 连接登录管理器的信号
	connect(m_loginManager.get(), &LoginManager::loginStatusChanged,
		this, &ConfigVideoPlatform::qrCodeLoginStatusChanged);
	connect(m_loginManager.get(), &LoginManager::loginSuccess,
		this, &ConfigVideoPlatform::qrCodeLoginSuccess);
	connect(m_loginManager.get(), &LoginManager::loginFailed,
		this, &ConfigVideoPlatform::qrCodeLoginFailed);
	connect(m_loginManager.get(), &LoginManager::loginStateChanged,
		this, &ConfigVideoPlatform::loginStateChanged);
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

QList<VideoInfo> ConfigVideoPlatform::getVideoInfo(const QString& url)
{
	LOG_INFO("ConfigVideoPlatform", "Getting video info for: %s", url.toUtf8().constData());

	QString videoId = extractVideoId(url);
	QString apiUrl = m_modInfo.getApiEndpoint("videoInfo");

	if (apiUrl.isEmpty())
	{
		LOG_ERROR("ConfigVideoPlatform", "Failed to get video info: %s", "视频信息API未设置");
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, "视频信息API未设置");
		return {};
	}

	// 构建请求参数
	QVariantMap params;
	QVariantMap headers = m_modInfo.getRequestHeaders();

	// 根据平台构建不同的参数
	params[m_modInfo.getConfigValue("apiParameters.videoInfo").toString()] = videoId;

	// 发送请求
	NetworkResponse response;
	if (!params.isEmpty())
	{
		QUrl fullUrl(apiUrl);
		QUrlQuery query;
		for (auto it = params.begin(); it != params.end(); ++it)
			query.addQueryItem(it.key(), it.value().toString());
		fullUrl.setQuery(query);
		response = m_networkManager->getWithLoop(fullUrl.toString(), headers);
	}
	else
	{
		response = m_networkManager->getWithLoop(apiUrl, headers);
	}

	if (!response.success)
	{
		LOG_ERROR("ConfigVideoPlatform", "Failed to get video info: %s", response.errorString);
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, response.errorString);
		return {};
	}

	QJsonDocument doc = QJsonDocument::fromJson(response.data);
	if (doc.isNull())
	{
		LOG_ERROR("ConfigVideoPlatform", "Failed to get video info: %s", "JSON数据异常!");
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, "JSON数据异常!");
		return {};
	}

	return parseVideoInfo(doc.object());
}

void ConfigVideoPlatform::getVideoCover(DownloadTaskInfo& taskInfo)
{
	QVariantMap headers = m_modInfo.getRequestHeaders();
	taskInfo.videoInfo.cover = m_networkManager->getWithLoop(taskInfo.videoInfo.thumbnailUrl.toString(), headers).data;
}

QUrl ConfigVideoPlatform::getVideoPlayUrl(StreamRequest& request)
{
	QString apiUrl = m_modInfo.getApiEndpoint("playUrl");
	if (apiUrl.isEmpty()) {
		throw std::runtime_error("Play URL API endpoint not configured");
	}

	// 构建请求参数
	QVariantMap params;
	QVariantMap headers = m_modInfo.getRequestHeaders();
	if (isLoggedIn())
	{
		QVariantMap cookies = getCookie();
		if (!cookies.isEmpty())
		{
			QStringList cookieList;
			for (auto it = cookies.begin(); it != cookies.end(); ++it)
			{
				cookieList.append(it.key() + "=" + it.value().toString());
			}
			QString cookiesStr = cookieList.join("; ");
			headers["Cookie"] = cookiesStr;
		}
	}

	// 设置清晰度
	//QVariantMap qualityMapping = m_modInfo.getQualityMapping(request.type);
	//params["qn"] = qualityMapping.value(request.quality, 64); // 默认 720p

	params.insert(request.extraParams);
	QVariantMap requestParams = m_modInfo.getConfigValue("qualityMapping.video").toMap();

	//判断清晰度
	//params.insert(requestParams.value(request.quality).toMap());
	params.insert(requestParams.value("8K").toMap());

	// 发送请求
	NetworkResponse response;
	if (!params.isEmpty())
	{
		QUrl fullUrl(apiUrl);
		QUrlQuery query;
		for (auto it = params.begin(); it != params.end(); ++it) {
			query.addQueryItem(it.key(), it.value().toString());
		}
		fullUrl.setQuery(query);
		response = m_networkManager->getWithLoop(fullUrl.toString(), headers);
	}
	else
	{
		response = m_networkManager->getWithLoop(apiUrl, headers);
	}

	if (!response.success) {
		throw std::runtime_error(response.errorString.toStdString());
	}

	QJsonDocument doc = QJsonDocument::fromJson(response.data);

	// 获取对象
	QJsonObject obj = doc.object();
	QUrl videoPlayUrl = parseVideoPlayUrl(doc.object());
	LOG_INFO("ConfigVideoPlatform", "Video Play Url retrieved: %s", videoPlayUrl.toUtf8().constData());

	return videoPlayUrl;
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
			NetworkResponse response = m_networkManager->get(apiUrl, headers);
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
			NetworkResponse response = m_networkManager->get(apiUrl, headers);
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
			NetworkResponse response = m_networkManager->get(apiUrl, headers);
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

QUrl ConfigVideoPlatform::parseVideoPlayUrl(const QJsonObject& data)
{
	QString urlPath = m_modInfo.getConfigValue("streamParser.video.urlPath").toString();
	//选择正确的清晰度
	urlPath.replace("#", "0");
	QUrl videoUrl(extractJsonValue(data, urlPath).toString());

	LOG_INFO("ConfigVideoPlatform", "Successfully parsed video URL: %s", videoUrl.toUtf8().constData());
	return videoUrl;
}

QList<VideoInfo> ConfigVideoPlatform::parseVideoInfo(const QJsonObject& data)
{
	QList<VideoInfo> videoList;
	QVariantMap parserConfig = m_modInfo.getVideoInfoParser();

	// 检查是否为哔哩哔哩平台
	if (m_modInfo.modId == "bilibili") {
		// 解析主视频信息
		VideoInfo mainInfo;
		mainInfo.title = extractJsonValue(data, parserConfig.value("title").toString()).toString();
		mainInfo.author = extractJsonValue(data, parserConfig.value("author").toString()).toString();
		mainInfo.description = extractJsonValue(data, parserConfig.value("description").toString()).toString();

		QVariant durationValue = extractJsonValue(data, parserConfig.value("duration").toString());
		if (durationValue.canConvert<int>()) {
			mainInfo.duration = mainInfo.formattedDuration(durationValue.toInt());
		}

		QString thumbnailUrl = extractJsonValue(data, parserConfig.value("thumbnail").toString()).toString();
		if (!thumbnailUrl.isEmpty()) {
			mainInfo.thumbnailUrl = QUrl(thumbnailUrl);
		}

		mainInfo.videoId = extractJsonValue(data, parserConfig.value("videoId").toString()).toString();

		// 添加额外参数
		mainInfo.extraParams["avid"] = extractJsonValue(data, "data.aid").toString();
		mainInfo.extraParams["cid"] = extractJsonValue(data, "data.cid").toString();

		// 解析统计数据
		QJsonObject statData = extractJsonValue(data, "data.stat").toJsonObject();
		if (!statData.isEmpty()) {
			mainInfo.viewCount = extractJsonValue(statData, "view").toLongLong();
			mainInfo.likeCount = extractJsonValue(statData, "like").toLongLong();
		}

		// 解析上传时间
		QVariant pubdateValue = extractJsonValue(data, "data.pubdate");
		if (pubdateValue.canConvert<qint64>()) {
			qint64 timestamp = pubdateValue.toLongLong();
			mainInfo.uploadDate = QDateTime::fromSecsSinceEpoch(timestamp);
		}
		mainInfo.platformId = m_modInfo.modId;
		videoList.append(mainInfo);

		// 解析分P信息
		QJsonArray pagesArray = extractJsonArray(data, "data.pages");
		if (!pagesArray.isEmpty() && pagesArray.size() > 1) {
			for (const QJsonValue& pageValue : pagesArray) {
				QJsonObject pageObj = pageValue.toObject();

				VideoInfo pageInfo;
				pageInfo.title = pageObj["part"].toString();
				pageInfo.author = mainInfo.author;
				pageInfo.description = mainInfo.description;


				pageInfo.thumbnailUrl = pageObj["first_frame"].toString().isEmpty() ? mainInfo.thumbnailUrl : pageObj["first_frame"].toString();

				pageInfo.videoId = mainInfo.videoId;
				pageInfo.duration = pageInfo.formattedDuration(pageObj["duration"].toInt());
				pageInfo.viewCount = mainInfo.viewCount;
				pageInfo.likeCount = mainInfo.likeCount;
				pageInfo.uploadDate = pageObj["ctime"].toInteger() ? QDateTime::fromSecsSinceEpoch(pageObj["ctime"].toInteger()) : mainInfo.uploadDate;

				// 添加分P特定参数
				pageInfo.extraParams["avid"] = mainInfo.extraParams["aid"];
				pageInfo.extraParams["cid"] = QString::number(pageObj["cid"].toVariant().toLongLong());

				pageInfo.platformId = m_modInfo.modId;
				videoList.append(pageInfo);
			}
		}

		// 解析合集信息（ugc_season）
		QJsonObject ugcSeason = extractJsonValue(data, "data.ugc_season").toJsonObject();
		if (!ugcSeason.isEmpty()) {
			QJsonArray sectionsArray = ugcSeason["sections"].toArray();

			for (const QJsonValue& sectionValue : sectionsArray) {
				QJsonObject sectionObj = sectionValue.toObject();
				QJsonArray episodesArray = sectionObj["episodes"].toArray();

				for (const QJsonValue& episodeValue : episodesArray) {
					QJsonObject episodeObj = episodeValue.toObject();
					QJsonObject arcObj = episodeObj["arc"].toObject();

					VideoInfo episodeInfo;
					episodeInfo.title = arcObj["title"].toString();
					if (episodeInfo.title == mainInfo.title)
					{
						continue;
					}
					episodeInfo.author = arcObj["author"].toObject()["name"].toString();
					episodeInfo.description = arcObj["desc"].toString();
					episodeInfo.uploadDate = QDateTime::fromSecsSinceEpoch(arcObj["pubdate"].toInteger());

					QString episodeThumbnail = arcObj["pic"].toString();
					if (!episodeThumbnail.isEmpty()) {
						episodeInfo.thumbnailUrl = QUrl(episodeThumbnail);
					}

					episodeInfo.videoId = episodeObj["bvid"].toString();
					episodeInfo.duration = episodeInfo.formattedDuration(arcObj["duration"].toInt());

					QJsonObject episodeStat = arcObj["stat"].toObject();
					episodeInfo.viewCount = episodeStat["view"].toVariant().toLongLong();
					episodeInfo.likeCount = episodeStat["like"].toVariant().toLongLong();
					episodeInfo.platformId = m_modInfo.modId;
					// 添加合集视频参数
					episodeInfo.extraParams["avid"] = QString::number(episodeObj["aid"].toVariant().toLongLong());
					episodeInfo.extraParams["cid"] = QString::number(episodeObj["cid"].toVariant().toLongLong());

					episodeInfo.platformId = m_modInfo.modId;
					videoList.append(episodeInfo);
				}
			}
		}
	}
	else {
		// 其他平台的原有逻辑（保持兼容）
		VideoInfo info;
		info.title = extractJsonValue(data, parserConfig.value("title").toString()).toString();
		info.author = extractJsonValue(data, parserConfig.value("author").toString()).toString();
		info.description = extractJsonValue(data, parserConfig.value("description").toString()).toString();

		QVariant durationValue = extractJsonValue(data, parserConfig.value("duration").toString());
		if (durationValue.canConvert<int>()) {
			info.duration = info.formattedDuration(durationValue.toInt());
		}

		QString thumbnailUrl = extractJsonValue(data, parserConfig.value("thumbnail").toString()).toString();
		if (!thumbnailUrl.isEmpty()) {
			info.thumbnailUrl = QUrl(thumbnailUrl);
		}

		info.videoId = extractJsonValue(data, parserConfig.value("videoId").toString()).toString();
		videoList.append(info);
	}

	return videoList;
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
	QList<QRegularExpression> regexs;
	QList<QString> urlPatterns = m_modInfo.getUrlPatterns();
	for (const QString& pattern : urlPatterns)
	{
		regexs.append(QRegularExpression(pattern));
	}
	// 根据平台提取视频ID
	for (const QRegularExpression& regex : regexs)
	{
		auto match = regex.match(url);
		if (match.hasMatch())
			return match.captured(1);
	}

	return QString();
}

QVariantMap ConfigVideoPlatform::getQualityParams(const QString& qualityName, StreamType type) const
{
	QVariantMap params;

	// 从 mod.json 获取质量映射
	QVariantMap qualityMapping = m_modInfo.getQualityMapping(type);

	// 获取指定画质的参数配置
	QVariant qualityConfig = qualityMapping.value(qualityName);

	if (qualityConfig.isValid() && qualityConfig.canConvert<QVariantMap>())
	{
		// 如果是对象形式，直接使用所有参数
		params = qualityConfig.toMap();
		params.remove("description"); // 移除 description 参数
	}

	return params;
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

	// 从配置文件获取画质参数
	QVariantMap qualityParams = getQualityParams(request.quality, request.type);
	params.insert(qualityParams);

	// 如果请求中有额外参数，覆盖配置文件的参数
	if (!request.extraParams.isEmpty()) {
		params.insert(request.extraParams);
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