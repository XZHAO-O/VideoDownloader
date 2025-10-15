#include "ModInfo.h"
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>

ModInfo ModInfo::fromJson(const QJsonObject& json, const QString& filePath)
{
	ModInfo info;
	info.modId = json["modId"].toString();
	info.name = json["name"].toString();
	info.version = json["version"].toString();
	info.author = json["author"].toString();
	info.description = json["description"].toString();
	info.config = json;
	info.modPath = filePath;
	info.loadTime = QDateTime::currentDateTime();

	// 解析支持的平台
	if (json.contains("supportedPlatforms")) {
		QJsonArray platforms = json["supportedPlatforms"].toArray();
		for (const auto& platform : platforms) {
			info.supportedPlatforms.append(platform.toString());
		}
	}

	// 解析URL模式
	if (json.contains("urlPatterns")) {
		QJsonArray patterns = json["urlPatterns"].toArray();
		for (const auto& pattern : patterns) {
			info.urlPatterns.append(pattern.toString());
		}
	}

	// 解析启用状态和优先级
	info.enabled = json.value("enabled").toBool();
	info.priority = json.value("priority").toInt();

	return info;
}

QJsonObject ModInfo::toJson() const
{
	QJsonObject json;
	json["modId"] = modId;
	json["name"] = name;
	json["version"] = version;
	json["author"] = author;
	json["description"] = description;
	json["enabled"] = enabled;
	json["priority"] = priority;

	// 序列化支持的平台
	QJsonArray platforms;
	for (const QString& platform : supportedPlatforms) {
		platforms.append(platform);
	}
	json["supportedPlatforms"] = platforms;

	// 序列化URL模式
	QJsonArray patterns;
	for (const QString& pattern : urlPatterns) {
		patterns.append(pattern);
	}
	json["urlPatterns"] = patterns;

	// 合并配置
	for (auto it = config.begin(); it != config.end(); ++it) {
		if (!json.contains(it.key())) {
			json[it.key()] = it.value();
		}
	}

	return json;
}

QVariant ModInfo::getConfigValue(const QString& key, const QVariant& defaultValue) const
{
	if (config.contains(key)) {
		return config.value(key).toVariant();
	}

	// 支持嵌套键，如 "apiEndpoints.videoInfo"
	QStringList keys = key.split('.');
	QJsonObject current = config;

	for (int i = 0; i < keys.size() - 1; ++i) {
		if (current.contains(keys[i]) && current[keys[i]].isObject()) {
			current = current[keys[i]].toObject();
		}
		else {
			return defaultValue;
		}
	}

	QString lastKey = keys.last();
	if (current.contains(lastKey)) {
		return current[lastKey].toVariant();
	}

	return defaultValue;
}

QString ModInfo::getApiEndpoint(const QString& endpointName) const
{
	return getConfigValue(QString("apiEndpoints.%1").arg(endpointName)).toString();
}

QVariantMap ModInfo::getRequestHeaders() const
{
	return getConfigValue("requestHeaders").toMap();
}

QVariantMap ModInfo::getQualityMapping(StreamType type) const
{
	QString typeKey = (type == StreamType::Video) ? "video" : "audio";
	return getConfigValue(QString("qualityMapping.%1").arg(typeKey)).toMap();
}

QStringList ModInfo::getUrlPatterns() const
{
	return urlPatterns;
}

QVariantMap ModInfo::getVideoInfoParser() const
{
	return getConfigValue("videoInfoParser").toMap();
}

QVariantMap ModInfo::getStreamParser(StreamType type) const
{
	QString typeKey = (type == StreamType::Video) ? "video" : "audio";
	QVariantMap streamParser = getConfigValue("streamParser").toMap();
	return streamParser.value(typeKey).toMap();
}