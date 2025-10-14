#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QDateTime>
#include <QVariant>
#include <QRegularExpression>
#include "VideoInfo.h"
#include "SearchResult.h"

// 流类型枚举
enum class StreamType {
	Video,
	Audio
};

// 流请求结构
struct StreamRequest {
	QString quality;
	StreamType type;
	QVariantMap extraParams;

	StreamRequest() : type(StreamType::Video) {}
	StreamRequest(const QString& qual, StreamType t) : quality(qual), type(t) {}
};

// Mod信息结构
struct ModInfo {
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
	QVariantMap getQualityMapping(StreamType type) const;

	// 获取URL模式列表
	QStringList getUrlPatterns() const;

	// 获取视频信息解析器配置
	QVariantMap getVideoInfoParser() const;

	// 获取流解析器配置
	QVariantMap getStreamParser(StreamType type) const;
};

Q_DECLARE_METATYPE(ModInfo)
Q_DECLARE_METATYPE(StreamType)
Q_DECLARE_METATYPE(StreamRequest)