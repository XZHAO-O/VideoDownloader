#pragma once

#include <QObject>
#include <QFuture>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include "ModInfo.h"
#include "INetworkManager.h"
#include "LogSystem.h"

class ConfigManager;

class ConfigVideoPlatform : public QObject
{
	Q_OBJECT

public:
	explicit ConfigVideoPlatform(const ModInfo& modInfo,
		QSharedPointer<INetworkManager> networkManager,
		QObject* parent = nullptr);

	// 平台接口
	VideoInfo getVideoInfo(const QString& url);
	QFuture<QList<StreamInfo>> getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<QList<StreamInfo>> getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<SearchResult> searchVideos(const QString& keyword, int page = 1);

	QString getModId() const { return m_modInfo.modId; }
	QString getName() const { return m_modInfo.name; }
	bool isEnabled() const { return m_modInfo.enabled; }
	bool matchesUrl(const QString& url) const;

signals:
	void videoInfoReceived(const VideoInfo& videoInfo);
	void streamsReceived(const QList<StreamInfo>& streams);
	void searchResultsReceived(const SearchResult& results);
	void errorOccurred(const QString& error);

private:
	// 内部方法
	QList<StreamInfo> parseStreams(const QJsonObject& data, StreamType type);
	QString extractVideoId(const QString& url);
	QVariantMap buildRequestParams(const VideoInfo& videoInfo, const StreamRequest& request);
	QVariant extractJsonValue(const QJsonObject& data, const QString& path);
	QJsonArray extractJsonArray(const QJsonObject& data, const QString& path);
	VideoInfo parseVideoInfo(const QJsonObject& data);

	ModInfo m_modInfo;
	QSharedPointer<INetworkManager> m_networkManager;
};