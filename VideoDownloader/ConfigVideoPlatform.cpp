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
	, m_qrCodePollTimer(new QTimer(this))
	, m_isQRCodeLoginActive(false)
{
	// 初始化二维码轮询定时器
	m_qrCodePollTimer->setInterval(3000); // 3秒轮询一次
	connect(m_qrCodePollTimer, &QTimer::timeout, this, &ConfigVideoPlatform::onQRCodePollTimeout);
}

// 获取二维码配置
QVariantMap ConfigVideoPlatform::getQRCodeConfig() const
{
	return m_modInfo.getConfigValue("qrCode").toMap();
}

// 开始二维码登录
QUrl ConfigVideoPlatform::startQRCodeLogin()
{
	QVariantMap qrConfig = getQRCodeConfig();
	if (!qrConfig.value("enabled", false).toBool()) {
		LOG_WARN("ConfigVideoPlatform", "QR code login is not enabled in configuration");
		emit qrCodeLoginFailed("二维码登录功能未启用");
		return QUrl();
	}

	if (m_isQRCodeLoginActive) {
		LOG_WARN("ConfigVideoPlatform", "QR code login is already active");
		return QUrl();
	}

	m_isQRCodeLoginActive = true;
	m_qrCodeKey.clear();

	LOG_INFO("ConfigVideoPlatform", "Starting QR code login");
	return generateQRCode();
}

// 停止二维码登录
void ConfigVideoPlatform::stopQRCodeLogin()
{
	if (m_qrCodePollTimer->isActive()) {
		m_qrCodePollTimer->stop();
	}

	m_isQRCodeLoginActive = false;
	m_qrCodeKey.clear();

	LOG_INFO("ConfigVideoPlatform", "QR code login stopped");
}

// 检查二维码登录是否活跃
bool ConfigVideoPlatform::isQRCodeLoginActive() const
{
	return m_isQRCodeLoginActive;
}

// 生成二维码
QUrl ConfigVideoPlatform::generateQRCode()
{
	try {
		QVariantMap qrConfig = getQRCodeConfig();
		QString generateUrl = qrConfig.value("generateUrl").toString();

		if (generateUrl.isEmpty()) {
			throw std::runtime_error("QR code generate URL not configured");
		}

		QVariantMap headers = m_modInfo.getRequestHeaders();

		// 发送请求生成二维码
		NetworkResponse response = m_networkManager->get(generateUrl, headers);

		if (!response.success) {
			throw std::runtime_error(response.errorString.toStdString());
		}

		QJsonDocument doc = QJsonDocument::fromJson(response.data);
		if (doc.isNull()) {
			throw std::runtime_error("Invalid JSON response");
		}

		QJsonObject rootObj = doc.object();
		if (rootObj["code"].toInt() != 0) {
			throw std::runtime_error(QString("API error: %1").arg(rootObj["message"].toString()).toStdString());
		}

		QJsonObject data = rootObj["data"].toObject();

		// 从配置中获取字段映射
		QString codeUrlPath = qrConfig.value("codeUrl").toString();
		QVariantMap checkParams = qrConfig.value("checkParams").toMap();

		// 提取二维码URL和key
		QUrl qrCodeUrl = QUrl(extractJsonValue(rootObj, codeUrlPath).toString());
		m_qrCodeKey = extractJsonValue(rootObj, checkParams.value("qrcode_key").toString()).toString();

		if (qrCodeUrl.isEmpty() || m_qrCodeKey.isEmpty()) {
			throw std::runtime_error("Failed to extract QR code data from response");
		}

		LOG_INFO("ConfigVideoPlatform", "QR code generated, key: %s", m_qrCodeKey.toUtf8().constData());

		// 创建二维码图片
		//QPixmap qrCodePixmap = SimpleQRCodeGenerator::generateQRCode(qrCodeUrl, 240);

		// 发出二维码生成信号
		//emit qrCodeGenerated(qrCodePixmap, m_qrCodeKey);

		// 获取状态码配置
		QVariantMap statusCodes = qrConfig.value("statusCode").toMap();
		int waitingCode = statusCodes.value("waiting").toInt();
		emit qrCodeLoginStatusChanged("请使用哔哩哔哩APP扫描二维码", waitingCode);

		// 开始轮询状态
		m_qrCodePollTimer->start();

		return qrCodeUrl;
	}
	catch (const std::exception& e) {
		LOG_ERROR("ConfigVideoPlatform", "Failed to generate QR code: %s", e.what());
		emit qrCodeLoginFailed(QString("生成二维码失败: %1").arg(e.what()));
		m_isQRCodeLoginActive = false;
	}
}

// 轮询二维码状态
void ConfigVideoPlatform::pollQRCodeStatus()
{
	if (m_qrCodeKey.isEmpty()) {
		return;
	}

	try {
		QVariantMap qrConfig = getQRCodeConfig();
		QString checkUrl = qrConfig.value("checkUrl").toString();

		if (checkUrl.isEmpty()) {
			throw std::runtime_error("QR code check URL not configured");
		}

		QVariantMap headers = m_modInfo.getRequestHeaders();

		// 构建查询参数
		QUrl url(checkUrl);
		QUrlQuery query;
		QVariantMap checkParams = qrConfig.value("checkParams").toMap();

		for (auto it = checkParams.begin(); it != checkParams.end(); ++it) {
			if (it.key() == "qrcode_key") {
				query.addQueryItem(it.key(), m_qrCodeKey);
			}
			// 可以添加其他参数
		}

		url.setQuery(query);

		NetworkResponse response = m_networkManager->get(url.toString(), headers);

		if (!response.success) {
			throw std::runtime_error(response.errorString.toStdString());
		}

		QJsonDocument doc = QJsonDocument::fromJson(response.data);
		if (doc.isNull()) {
			throw std::runtime_error("Invalid JSON response");
		}

		QJsonObject rootObj = doc.object();
		if (rootObj["code"].toInt() != 0) {
			throw std::runtime_error(QString("API error: %1").arg(rootObj["message"].toString()).toStdString());
		}

		QJsonObject data = rootObj["data"].toObject();

		// 从配置中获取状态字段和状态码
		QString statusPath = qrConfig.value("status").toString();
		QVariantMap statusCodes = qrConfig.value("statusCode").toMap();

		int statusCode = extractJsonValue(data, statusPath).toInt();
		QString message = data["message"].toString();

		LOG_INFO("ConfigVideoPlatform", "QR code status: %d - %s", statusCode, message.toUtf8().constData());

		// 根据状态码处理不同情况
		if (statusCode == statusCodes.value("success").toInt()) {
			// 登录成功
			handleQRCodeLoginSuccess(data);
		}
		else if (statusCode == statusCodes.value("expired").toInt()) {
			// 二维码过期
			emit qrCodeLoginStatusChanged("二维码已失效，请重新扫描", statusCode);
			stopQRCodeLogin();
		}
		else if (statusCode == statusCodes.value("unconfirmed").toInt()) {
			// 已扫描未确认
			emit qrCodeLoginStatusChanged("扫码成功，请在手机上确认登录", statusCode);
		}
		else if (statusCode == statusCodes.value("waiting").toInt()) {
			// 等待扫描
			emit qrCodeLoginStatusChanged("请使用哔哩哔哩APP扫描二维码", statusCode);
		}
		else {
			// 其他状态
			emit qrCodeLoginStatusChanged(message, statusCode);
		}

	}
	catch (const std::exception& e) {
		LOG_ERROR("ConfigVideoPlatform", "Failed to poll QR code status: %s", e.what());
		// 不停止轮询，继续尝试
	}
}

// 处理二维码登录成功
void ConfigVideoPlatform::handleQRCodeLoginSuccess(const QJsonObject& data)
{
	try {
		QString url = data["url"].toString();
		QString refreshToken = data["refresh_token"].toString();
		qint64 timestamp = data["timestamp"].toVariant().toLongLong();

		LOG_INFO("ConfigVideoPlatform", "QR code login success, timestamp: %lld", timestamp);

		// 解析URL获取cookies
		QUrl loginUrl(url);
		QUrlQuery query(loginUrl.query());

		QVariantMap authData;
		authData["refresh_token"] = refreshToken;
		authData["timestamp"] = timestamp;
		authData["DedeUserID"] = query.queryItemValue("DedeUserID");
		authData["DedeUserID__ckMd5"] = query.queryItemValue("DedeUserID__ckMd5");
		authData["SESSDATA"] = query.queryItemValue("SESSDATA");
		authData["bili_jct"] = query.queryItemValue("bili_jct");
		authData["Expires"] = query.queryItemValue("Expires");

		// 停止轮询
		stopQRCodeLogin();

		// 发出成功信号
		emit qrCodeLoginStatusChanged("登录成功", 0);
		emit qrCodeLoginSuccess(authData);

		LOG_INFO("ConfigVideoPlatform", "QR code login completed successfully");

	}
	catch (const std::exception& e) {
		LOG_ERROR("ConfigVideoPlatform", "Failed to handle QR code login success: %s", e.what());
		emit qrCodeLoginFailed(QString("处理登录成功数据失败: %1").arg(e.what()));
	}
}

// 获取状态码
int ConfigVideoPlatform::getQRStatusCode(const QString& status)
{
	QVariantMap qrConfig = getQRCodeConfig();
	QVariantMap statusCodes = qrConfig.value("statusCode").toMap();

	if (status == "success") return statusCodes.value("success").toInt();
	if (status == "unconfirmed") return statusCodes.value("unconfirmed").toInt();
	if (status == "waiting") return statusCodes.value("waiting").toInt();
	if (status == "expired") return statusCodes.value("expired").toInt();

	return -1;
}

// 定时器超时处理
void ConfigVideoPlatform::onQRCodePollTimeout()
{
	if (m_isQRCodeLoginActive && !m_qrCodeKey.isEmpty()) {
		pollQRCodeStatus();
	}
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
		params[m_modInfo.getConfigValue("apiParameters.videoInfo").toString()] = videoId;

		// 发送请求
		NetworkResponse response;
		if (!params.isEmpty()) {
			QUrl fullUrl(apiUrl);
			QUrlQuery query;
			for (auto it = params.begin(); it != params.end(); ++it) {
				query.addQueryItem(it.key(), it.value().toString());
			}
			fullUrl.setQuery(query);
			response = m_networkManager->get(fullUrl.toString(), headers);
		}
		else {
			response = m_networkManager->get(apiUrl, headers);
		}

		if (!response.success) {
			throw std::runtime_error(response.errorString.toStdString());
		}

		QJsonDocument doc = QJsonDocument::fromJson(response.data);
		if (doc.isNull()) {
			throw std::runtime_error("Invalid JSON response");
		}
		//qDebug() << doc.toJson();
		QList<VideoInfo> videoInfoList = parseVideoInfo(doc.object());
		for (VideoInfo& videoInfo : videoInfoList)
		{
			videoInfo.platformId = m_modInfo.modId;

			LOG_INFO("ConfigVideoPlatform", "Video info retrieved: %s", videoInfo.title.toUtf8().constData());
			emit videoInfoReceived(videoInfo);
		}

		return videoInfoList;

	}
	catch (const std::exception& e) {
		LOG_ERROR("ConfigVideoPlatform", "Failed to get video info: %s", e.what());
		emit errorOccurred(QString("Failed to get video info: %1").arg(e.what()));
		throw;
	}
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
	//QString cookies = getCookies();
	//if (!cookies.isEmpty()) {
	//	headers["Cookie"] = cookies;
	//}

	// 设置清晰度
	//QVariantMap qualityMapping = m_modInfo.getQualityMapping(request.type);
	//params["qn"] = qualityMapping.value(request.quality, 64); // 默认 720p


	// 根据平台构建不同的参数
	request.extraParams["qn"] = 80;
	request.extraParams["type"] = "mp4";
	request.extraParams["platform"] = "html5";
	request.extraParams["high_quality"] = 1;

	params.insert(request.extraParams);

	// 发送请求
	NetworkResponse response;
	if (!params.isEmpty()) {
		QUrl fullUrl(apiUrl);
		QUrlQuery query;
		for (auto it = params.begin(); it != params.end(); ++it) {
			query.addQueryItem(it.key(), it.value().toString());
		}
		fullUrl.setQuery(query);
		response = m_networkManager->get(fullUrl.toString(), headers);
	}
	else {
		response = m_networkManager->get(apiUrl, headers);
	}

	if (!response.success) {
		throw std::runtime_error(response.errorString.toStdString());
	}

	QJsonDocument doc = QJsonDocument::fromJson(response.data);
	qDebug() << "JSON Document:" << doc.toJson();

	// 获取对象
	QJsonObject obj = doc.object();
	QUrl videoPlayUrl = parseVideoPlayUrl(doc.object());
	LOG_INFO("ConfigVideoPlatform", "Video Play Url retrieved: %s", videoPlayUrl.toUtf8().constData());
	//emit videoInfoReceived(videoInfo);

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
	QString urlPath = m_modInfo.getConfigValue("streamParser.url").toString();

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