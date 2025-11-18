#pragma once

#include <QJsonObject>

// 流请求结构
struct StreamRequest {
	QString quality;
	//StreamType type;
	QVariantMap extraParams;

	StreamRequest() {}
	StreamRequest(const QString& qual) : quality(qual) {}
};

// Mod信息结构
struct ModInfo
{
	QString modId;
	QString name;
	QString version;
	QString author;
	QString description;
	QStringList supportedPlatforms;
	QStringList urlPatterns;
	QJsonObject config;
	bool enabled = true;
	int priority = 1;
	QDateTime loadTime;
	QString modPath;

	// 从配置文件加载
	static ModInfo fromJson(const QJsonObject& json, const QString& filePath);

	// 转换为JSON
	QJsonObject toJson() const;

	bool isValid() const { return !modId.isEmpty() && !name.isEmpty(); }

	// 获取配置值
	QVariant getConfigValue(const QString& key, const QVariant& defaultValue = QVariant()) const;

	// 获取API端点
	QString getApiEndpoint(const QString& endpointName) const;

	// 获取请求头
	QVariantMap getRequestHeaders() const;

	// 获取质量映射
	QVariantMap getVideoQualityMapping() const;

	QVariantMap getAudioQualityMapping() const;

	// 获取URL模式列表
	QStringList getUrlPatterns() const;

	// 获取视频信息解析器配置
	QVariantMap getVideoInfoParser() const;

	// 获取流解析器配置
	QVariantMap getVideoStreamParser() const;

	QVariantMap getAudioStreamParser() const;
};

Q_DECLARE_METATYPE(ModInfo)
Q_DECLARE_METATYPE(StreamRequest)