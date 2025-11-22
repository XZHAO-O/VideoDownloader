#include "ConfigVideoPlatform.h"

#include <QtConcurrent/QtConcurrent>

#include "LogSystem.h"
#include "ConfigManager.h"
#include "NetworkManager.h"
#include "DownloadTaskInfo.h"
#include "AntMessageManager.h"
#include "Instrumentor.h"
#include "CancelManager.h"

ConfigVideoPlatform::ConfigVideoPlatform(const ModInfo& modInfo, QString modPath, QSharedPointer<NetworkManager> networkManager,
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

QJsonObject ConfigVideoPlatform::fetchVideoInfoApiResponse(const QUrl& url, const QVariantMap& params)
{
	if (url.isEmpty())
	{
		LOG_WARN("ConfigVideoPlatform", QString("Empty API URL: %1").arg(url.toString().toUtf8().constData()));
		return QJsonObject();
	}

	// 构建请求参数
	QVariantMap headers = m_modInfo.getRequestHeaders();

	// 发送请求
	NetworkResponse response;
	QUrl fullUrl(url);
	QUrlQuery query;
	for (auto it = params.begin(); it != params.end(); ++it)
	{
		query.addQueryItem(it.key(), it.value().toString());
	}
	fullUrl.setQuery(query);
	response = m_networkManager->getWithLoop(fullUrl, headers);

	if (!response.success)
	{
		LOG_WARN("ConfigVideoPlatform", QString("Failed to get video info for: %1").arg(response.errorString));
		return QJsonObject();
	}

	QJsonDocument doc = QJsonDocument::fromJson(response.data);
	if (doc.isNull())
	{
		LOG_WARN("ConfigVideoPlatform", QString("Invalid JSON response for: %1").arg(url.toString().toUtf8().constData()));
		return QJsonObject();
	}

	return doc.object();
}

QList<VideoInfo> ConfigVideoPlatform::getVideoInfo(const QString& url)
{
	LOG_INFO("ConfigVideoPlatform", QString("Getting video info for: %1").arg(url.toUtf8().constData()));

	QString videoId = extractVideoId(url);
	QVariantList apiEndpoints = m_modInfo.getConfigValue("apiEndpoints.videoInfo").toList();

	if (apiEndpoints.isEmpty())
	{
		LOG_ERROR("ConfigVideoPlatform", QString("Failed to get video info: %1").arg("视频信息API未设置"));
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, "视频信息API未设置");
		return {};
	}

	// 存储不同index的响应
	QMap<int, QJsonObject> responseMap;

	// 获取解析器配置
	QVariantMap parserConfig = m_modInfo.getVideoInfoParser();
	// 定义解析器检查顺序
	QStringList parserOrder = parserConfig.keys();
	QString selectedParser = "SingleVideo";

	auto getResponse = [&](const int& index) {
		auto it = responseMap.find(index);
		if (it == responseMap.end() || it.value().isEmpty())
		{
			QString apiUrl = apiEndpoints[index].toString();

			// 构建请求参数
			QVariantMap params;
			params[m_modInfo.getConfigValue("apiParameters.videoInfo").toString()] = videoId;

			auto doc = fetchVideoInfoApiResponse(apiUrl, params);

			responseMap[index] = doc;
		}
		};

	// 按顺序检查解析器
	for (const QString& parserName : parserOrder)
	{
		QVariantMap parser = parserConfig.value(parserName).toMap();
		if (parser.isEmpty())
			continue;
		QString method = parser.value("method").toString();

		if (method == "default")
		{
			// SingleVideo 是默认解析器，最后使用
			selectedParser = parserName;
			continue;
		}

		// 获取解析器需要的index
		int index = parser.value("index", 0).toInt();
		if (index >= apiEndpoints.size())
		{
			LOG_ERROR("ConfigVideoPlatform", QString("API endpoint index %1 out of range").arg(index));
			return {};
		}
		// 如果responseMap中没有这个index的响应，则发送请求
		getResponse(index);

		auto response = responseMap[index];
		// 检查解析器条件
		bool conditionMet = false;
		QString path = parser.value("path").toString();

		if (method == "exist")
		{
			// 检查路径是否存在
			QVariant value = extractJsonValue(response, path);
			conditionMet = !value.isNull();
		}
		else if (method == "check")
		{
			// 检查路径的值是否等于指定值
			QVariant value = extractJsonValue(response, path);
			QVariant expectedValue = parser.value("value");
			conditionMet = (value == expectedValue);
		}

		if (conditionMet)
		{
			selectedParser = parserName;
			break;
		}
	}

	if (selectedParser.isEmpty())
	{
		LOG_ERROR("ConfigVideoPlatform", QString("Failed to get video info: %1").arg("没有找到合适的解析器"));
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, "没有找到合适的解析器");
		return {};
	}

	QVariantMap parser = parserConfig.value(selectedParser).toMap();
	for (auto it = parser.begin(); it != parser.end(); ++it)
	{
		QVariant value = it.value();
		if (value.typeId() == QMetaType::QVariantMap)
		{
			QVariantMap subMap = value.toMap();

			// 检查子项中是否包含 "index"
			auto indexIt = subMap.find("index");
			if (indexIt != subMap.end())
			{
				int index = indexIt.value().toInt();
				if (index >= apiEndpoints.size())
				{
					LOG_ERROR("ConfigVideoPlatform", QString("API endpoint index %1 out of range").arg(index));
					return {};
				}
				getResponse(index);
				if (responseMap[index].isEmpty())
				{
					return {};
				}
			}
		}
	}

	// 使用选定的解析器解析视频信息
	return parseVideoInfo(responseMap, parser);
}

void ConfigVideoPlatform::getVideoCover(QSharedPointer<DownloadTaskInfo> taskInfo, const QString& cancelToken)
{
	QVariantMap headers = m_modInfo.getRequestHeaders();

	auto response = m_networkManager->getWithLoop(taskInfo->videoInfo.coverUrl, headers, cancelToken);
	if (response.success)
	{
		taskInfo->videoInfo.cover = response.data;
	}
}

void ConfigVideoPlatform::getDownloadInfo(QSharedPointer<DownloadTaskInfo> taskInfo, const QString& cancelToken)
{
	BENCHMARKING_FUNCTION();

	QVariantList apiEndpoints = m_modInfo.getConfigValue("apiEndpoints.downloadInfo").toList();
	QString apiUrl = apiEndpoints[0].toString();

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

	params.insert(taskInfo->videoInfo.extraParams);
	QVariantMap requestParams = m_modInfo.getConfigValue("qualityMapping.video").toMap();

	//判断清晰度
	//params.insert(requestParams.value(request.quality).toMap());
	params.insert(requestParams.value("8K").toMap());

	// 再次检查
	if (!cancelToken.isEmpty() && CancelManager::instance().isCancelled(cancelToken))
	{
		return;
	}

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

		response = m_networkManager->getWithLoop(fullUrl.toString(), headers, cancelToken);
	}
	else
	{
		response = m_networkManager->getWithLoop(apiUrl, headers, cancelToken);
	}

	if (!response.success)
	{
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, response.errorString);
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(response.data);
	qDebug() << doc.toJson();
	// 获取对象
	QJsonObject obj = doc.object();
	parseVideoPlayUrl(taskInfo, doc.object());

	//taskInfo->videoDownloadUrls["0"] = videoPlayUrl;

	auto keys = taskInfo->videoStreamInfo.keys();

	auto& streamInfo = taskInfo->videoStreamInfo[keys[0]];

	NetworkReplyHeader replyHeader = m_networkManager->getReplyHeaderWithLoop(streamInfo.url, headers, cancelToken);
	if (!replyHeader.success)
	{
		AntMessageManager::instance()->showMessage(AntMessage::Error, AntMessage::Singleton, replyHeader.errorString);
		return;
	}
	streamInfo.fileSize = replyHeader.getContentLength();
	//if (replyHeader.getAcceptRanges() == "bytes" && taskInfo->videoSizes["0"] > 0)
	//	taskInfo->partialDownloadSupport = true;
	//else
	//	taskInfo->partialDownloadSupport = false;
}

QFuture<QList<StreamInfo>> ConfigVideoPlatform::getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
		try {
			//LOG_INFO("ConfigVideoPlatform", QString("Getting video streams for: %1").arg(videoInfo.title.toUtf8().constData()));

			QString apiUrl = m_modInfo.getApiEndpoint("downloadInfo");
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

			QList<StreamInfo> streams = parseStreams(doc.object());

			//LOG_INFO("ConfigVideoPlatform", QString("Retrieved %1 video streams").arg(streams.size()));
			emit streamsReceived(streams);

			return streams;

		}
		catch (const std::exception& e) {
			//LOG_ERROR("ConfigVideoPlatform", "Failed to get video streams: %s", e.what());
			emit errorOccurred(QString("Failed to get video streams: %1").arg(e.what()));
			throw;
		}
		});
}

QFuture<QList<StreamInfo>> ConfigVideoPlatform::getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	//return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
	//	try {
	//		LOG_INFO("ConfigVideoPlatform", "Getting audio streams for: %s",
	//			videoInfo.title.toUtf8().constData());

	//		QString apiUrl = m_modInfo.getApiEndpoint("playUrl");
	//		if (apiUrl.isEmpty()) {
	//			throw std::runtime_error("Play URL API endpoint not configured");
	//		}

	//		// 构建请求参数
	//		QVariantMap params = buildRequestParams(videoInfo, request);
	//		QVariantMap headers = m_modInfo.getRequestHeaders();

	//		// 发送请求
	//		NetworkResponse response = m_networkManager->get(apiUrl, headers);
	//		if (!response.success) {
	//			throw std::runtime_error(response.errorString.toStdString());
	//		}

	//		QJsonDocument doc = QJsonDocument::fromJson(response.data);
	//		if (doc.isNull()) {
	//			throw std::runtime_error("Invalid JSON response");
	//		}

	//		QList<StreamInfo> streams = parseStreams(doc.object(), StreamType::Audio);

	//		LOG_INFO("ConfigVideoPlatform", "Retrieved %d audio streams", streams.size());
	//		emit streamsReceived(streams);

	//		return streams;

	//	}
	//	catch (const std::exception& e) {
	//		LOG_ERROR("ConfigVideoPlatform", "Failed to get audio streams: %s", e.what());
	//		emit errorOccurred(QString("Failed to get audio streams: %1").arg(e.what()));
	//		throw;
	//	}
	//	});
	return {};
}

//QFuture<SearchResult> ConfigVideoPlatform::searchVideos(const QString& keyword, int page)
//{
//	//return QtConcurrent::run([this, keyword, page]() -> SearchResult {
//	//	try {
//	//		LOG_INFO("ConfigVideoPlatform", "Searching videos: %s", keyword.toUtf8().constData());
//
//	//		QString apiUrl = m_modInfo.getApiEndpoint("search");
//	//		if (apiUrl.isEmpty()) {
//	//			throw std::runtime_error("Search API endpoint not configured");
//	//		}
//
//	//		// 构建请求参数
//	//		QVariantMap params;
//	//		QVariantMap headers = m_modInfo.getRequestHeaders();
//
//	//		// 根据平台构建搜索参数
//	//		if (m_modInfo.modId == "bilibili") {
//	//			params["keyword"] = keyword;
//	//			params["page"] = page;
//	//			params["search_type"] = "video";
//	//		}
//	//		else if (m_modInfo.modId == "youtube") {
//	//			params["q"] = keyword;
//	//			params["page"] = page;
//	//		}
//
//	//		// 发送请求
//	//		NetworkResponse response = m_networkManager->get(apiUrl, headers);
//	//		if (!response.success) {
//	//			throw std::runtime_error(response.errorString.toStdString());
//	//		}
//
//	//		QJsonDocument doc = QJsonDocument::fromJson(response.data);
//	//		if (doc.isNull()) {
//	//			throw std::runtime_error("Invalid JSON response");
//	//		}
//
//	//		// 解析搜索结果（简化实现）
//	//		SearchResult result;
//	//		result.searchQuery = keyword;
//	//		result.platformId = m_modInfo.modId;
//
//	//		// 这里需要根据具体平台的响应格式进行解析
//	//		// 暂时返回空结果，实际实现时需要根据平台API文档实现
//
//	//		LOG_INFO("ConfigVideoPlatform", "Search completed, found %d results", result.items.size());
//	//		emit searchResultsReceived(result);
//
//	//		return result;
//
//	//	}
//	//	catch (const std::exception& e) {
//	//		LOG_ERROR("ConfigVideoPlatform", "Failed to search videos: %s", e.what());
//	//		emit errorOccurred(QString("Failed to search videos: %1").arg(e.what()));
//	//		throw;
//	//	}
//	//	});
//	return {};
//}

void ConfigVideoPlatform::parseVideoPlayUrl(QSharedPointer<DownloadTaskInfo> task, const QJsonObject& data)
{
	QVariantMap path = m_modInfo.getConfigValue("streamParser.video").toMap();

	QStringList urls = extractJsonValue(data, path.value("urlPath").toString()).toStringList();
	QStringList codecs = extractJsonValue(data, path.value("codecPath").toString()).toStringList();
	QStringList qualities = extractJsonValue(data, path.value("qualityPath").toString()).toStringList();

	for (int i = 0; i < urls.size(); i++)
	{
		StreamInfo streamInfo;
		streamInfo.url = urls[i];
		streamInfo.codec = codecs[i];
		streamInfo.quality = qualities[i];
		task->videoStreamInfo.insert(qualities[i] + codecs[i], streamInfo);
	}

	path = m_modInfo.getConfigValue("streamParser.video").toMap();

	urls = extractJsonValue(data, path.value("urlPath").toString()).toStringList();
	codecs = extractJsonValue(data, path.value("codecPath").toString()).toStringList();
	qualities = extractJsonValue(data, path.value("qualityPath").toString()).toStringList();

	for (int i = 0; i < urls.size(); i++)
	{
		StreamInfo streamInfo;
		streamInfo.url = urls[i];
		streamInfo.codec = codecs[i];
		streamInfo.quality = qualities[i];
		task->audioStreamInfo.insert(qualities[i] + codecs[i], streamInfo);
	}

	LOG_INFO("ConfigVideoPlatform", QString("Successfully parsed video URL: %1"));
}

QList<VideoInfo> ConfigVideoPlatform::parseVideoInfo(const QMap<int, QJsonObject>& responseMap, const QVariantMap& parser)
{
	QList<VideoInfo> videoInfoList;

	// 提取title并转换为QStringList
	QVariantMap config = parser.value("title").toMap();
	QJsonObject data = responseMap.value(config.value("index").toInt());
	QVariant titleValue = extractJsonValue(data, config.value("path").toString());
	QStringList titleList = titleValue.canConvert<QStringList>()
		? titleValue.toStringList()
		: (QStringList() << titleValue.toString());

	int itemCount = titleList.size();
	if (itemCount == 0)
	{
		return videoInfoList; // 如果没有title，返回空列表
	}

	// 辅助函数：将QVariant转换为列表，如果无法转换则创建填充列表
	auto toVariantList = [itemCount](const QVariant& value) -> QVariantList {
		QVariantList result;

		if (value.typeId() == QMetaType::QVariantList)
		{
			QVariantList list = value.toList();
			if (list.size() == itemCount)
			{
				return list;
			}
			else if (list.size() > itemCount)
			{
				// 如果列表比title多，截取前itemCount个
				return list.mid(0, itemCount);
			}
			else
			{
				// 如果列表比title少，用最后一个元素填充
				result = list;
				while (result.size() < itemCount)
				{
					result.append(list.isEmpty() ? QVariant() : list.last());
				}
				return result;
			}
		}
		else
		{
			// 不是列表，创建填充列表
			for (int i = 0; i < itemCount; ++i)
			{
				result.append(value);
			}
			return result;
		}
		};

	// 提取videoId
	config = parser.value("videoId").toMap();
	data = responseMap.value(config.value("index").toInt());
	QVariant videoIdValue = extractJsonValue(data, config.value("path").toString());
	QVariantList videoIdList = toVariantList(videoIdValue);

	// 提取author
	config = parser.value("author").toMap();
	data = responseMap.value(config.value("index").toInt());
	QVariant authorValue = extractJsonValue(data, config.value("path").toString());
	QVariantList authorList = toVariantList(authorValue);

	// 提取duration
	config = parser.value("duration").toMap();
	data = responseMap.value(config.value("index").toInt());
	QVariant durationValue = extractJsonValue(data, config.value("path").toString());
	QVariantList durationList = toVariantList(durationValue);

	// 提取coverUrl
	config = parser.value("cover").toMap();
	data = responseMap.value(config.value("index").toInt());
	QVariant coverUrlValue = extractJsonValue(data, config.value("path").toString());
	QVariantList coverUrlList = toVariantList(coverUrlValue);

	// 提取publishTime
	config = parser.value("publishTime").toMap();
	data = responseMap.value(config.value("index").toInt());
	QVariant publishTimeValue = extractJsonValue(data, config.value("path").toString());
	QVariantList publishTimeList = toVariantList(publishTimeValue);

	// 提取sectionName（如果有）
	QStringList sectionNameList;
	if (parser.contains("sectionName"))
	{
		config = parser.value("sectionName").toMap();
		data = responseMap.value(config.value("index").toInt());
		QVariant sectionNameValue = extractJsonValue(data, config.value("path").toString());
		QVariantList sectionNameVariantList = toVariantList(sectionNameValue);
		for (const QVariant& item : sectionNameVariantList)
		{
			sectionNameList << item.toString();
		}
	}

	// 处理extraParams
	QVariantList extraParamsConfigs = parser.value("extraParams").toList();
	QList<QVariantMap> extraParamsList;

	// 初始化extraParams列表
	for (int i = 0; i < itemCount; ++i)
	{
		extraParamsList.append(QVariantMap());
	}

	// 为每个extraParam提取数据
	for (const QVariant& extraParamsConfig : extraParamsConfigs)
	{
		QVariantMap paramsConfig = extraParamsConfig.toMap();
		data = responseMap.value(paramsConfig.value("index").toInt());
		QVariant paramsValue = extractJsonValue(data, paramsConfig.value("path").toString());
		QString paramName = paramsConfig.value("name").toString();

		QVariantList paramList = toVariantList(paramsValue);

		// 将参数值设置到对应的extraParams中
		for (int i = 0; i < itemCount && i < paramList.size(); ++i)
		{
			QVariantMap& currentParams = extraParamsList[i];
			currentParams.insert(paramName, paramList[i]);
		}
	}

	// 创建VideoInfo对象
	for (int i = 0; i < itemCount; ++i)
	{
		VideoInfo info;
		info.sectionName = sectionNameList.isEmpty() ? QString() : sectionNameList.value(i);

		info.videoId = videoIdList.value(i).toString();
		info.title = titleList.value(i);
		info.author = authorList.value(i).toString();
		info.duration = StringUtil::formatDuration(durationList.value(i).toString());
		info.publishTime = StringUtil::formatDateTime(publishTimeList.value(i).toString());
		info.coverUrl = coverUrlList.value(i).toString();
		info.extraParams = extraParamsList.value(i);

		info.streamType = StreamType::AVMerged; // 默认值，可根据需要调整

		if (info.isValid())
		{
			videoInfoList.append(std::move(info));
		}
	}

	return videoInfoList;
}

QList<StreamInfo> ConfigVideoPlatform::parseStreams(const QJsonObject& data)
{
	QList<StreamInfo> streams;
	QVariantMap parserConfig = m_modInfo.getVideoStreamParser();

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

		//if (type == StreamType::Video) {
		//	QVariant widthValue = extractJsonValue(streamObj, parserConfig.value("widthPath").toString());
		//	QVariant heightValue = extractJsonValue(streamObj, parserConfig.value("heightPath").toString());
		//	if (widthValue.canConvert<int>() && heightValue.canConvert<int>()) {
		//		stream.width = widthValue.toInt();
		//		stream.height = heightValue.toInt();
		//	}
		//}

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

QVariantMap ConfigVideoPlatform::getQualityParams(const QString& qualityName) const
{
	QVariantMap params;

	// 从 mod.json 获取质量映射
	QVariantMap qualityMapping = m_modInfo.getVideoQualityMapping();

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
		QVariantMap qualityMapping = m_modInfo.getVideoQualityMapping();
		params["qn"] = qualityMapping.value(request.quality);
	}
	else if (m_modInfo.modId == "youtube") {
		params["videoId"] = videoInfo.videoId;
		// YouTube 参数构建
	}

	// 从配置文件获取画质参数
	QVariantMap qualityParams = getQualityParams(request.quality);
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

	for (int i = 0; i < keys.size(); ++i) {
		const QString& key = keys[i];

		if (current.isObject()) {
			current = current.toObject().value(key);
		}
		else if (current.isArray()) {
			QJsonArray array = current.toArray();

			// 处理 # 的情况 - 返回数组中所有元素的后续路径
			if (key == "#") {
				// 如果是最后一个键，直接返回整个数组
				if (i == keys.size() - 1) {
					return array.toVariantList();
				}

				// 否则，对数组中每个元素应用剩余的路径
				QVariantList results;
				QStringList remainingKeys = keys.mid(i + 1);

				for (const QJsonValue& item : array) {
					if (item.isObject() || item.isArray()) {
						QVariant result = extractJsonValue(item.toObject(), remainingKeys.join('.'));
						if (!result.isNull()) {
							results.append(result);
						}
					}
				}
				return results;
			}
			// 处理 #数字 的情况 - 返回数组中指定索引元素的后续路径
			else if (key.startsWith("#")) {
				bool ok;
				int index = key.mid(1).toInt(&ok); // 去掉 # 后解析数字

				if (ok && index >= 0 && index < array.size()) {
					current = array.at(index);
				}
				else {
					return QVariant();
				}
			}
			// 普通数字索引
			else {
				bool ok;
				int index = key.toInt(&ok);
				if (ok && index >= 0 && index < array.size()) {
					current = array.at(index);
				}
				else {
					return QVariant();
				}
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