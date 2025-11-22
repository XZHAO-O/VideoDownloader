#pragma once

#include <QFuture>
#include <QJsonArray>

#include "ModInfo.h"
#include "LoginManager.h"
#include "VideoInfo.h"

class NetworkManager;
class ConfigManager;
class DownloadTaskInfo;

class ConfigVideoPlatform : public QObject
{
	Q_OBJECT

public:
	explicit ConfigVideoPlatform(const ModInfo& modInfo, QString modPath, QSharedPointer<NetworkManager> networkManager, QObject* parent = nullptr);

	// 平台接口
	QList<VideoInfo> getVideoInfo(const QString& url);
	void getVideoCover(QSharedPointer<DownloadTaskInfo> taskInfo, const QString& cancelToken = QString());
	void getDownloadInfo(QSharedPointer<DownloadTaskInfo> taskInfo, const QString& cancelToken = QString());
	QFuture<QList<StreamInfo>> getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<QList<StreamInfo>> getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	//QFuture<SearchResult> searchVideos(const QString& keyword, int page = 1);
	void parseVideoPlayUrl(QSharedPointer<DownloadTaskInfo> task, const QJsonObject& data);

	// 登录相关功能（委托给 LoginManager）
	QUrl startQRCodeLogin() { return m_loginManager->startQRCodeLogin(); }
	void stopQRCodeLogin() { m_loginManager->stopQRCodeLogin(); }
	bool isQRCodeLoginActive() const { return m_loginManager->isQRCodeLoginActive(); }
	bool isLoggedIn() const { return m_loginManager->isLoggedIn(); }
	QVariantMap getCookie() const { return m_loginManager->getCookie(); }
	void clearCookies() { m_loginManager->clearCookies(); }

	QString getModId() const { return m_modInfo.modId; }
	QString getName() const { return m_modInfo.name; }
	bool isEnabled() const { return m_modInfo.enabled; }
	bool matchesUrl(const QString& url) const;

	QJsonObject fetchVideoInfoApiResponse(const QUrl& url, const QVariantMap& params);

signals:
	void videoInfoReceived(const VideoInfo& videoInfo);
	void streamsReceived(const QList<StreamInfo>& streams);
	//void searchResultsReceived(const SearchResult& results);
	void errorOccurred(const QString& error);

	// 登录相关信号（转发 LoginManager 的信号）
	void qrCodeLoginStatusChanged(const QString& status, int code);
	void qrCodeLoginSuccess(const QVariantMap& authData);
	void qrCodeLoginFailed(const QString& error);
	void loginStateChanged(bool isLoggedIn);

private:
	// 内部方法
	QList<StreamInfo> parseStreams(const QJsonObject& data);
	QString extractVideoId(const QString& url);
	QVariantMap getQualityParams(const QString& qualityName) const;
	QVariantMap buildRequestParams(const VideoInfo& videoInfo, const StreamRequest& request);
	QVariant extractJsonValue(const QJsonObject& data, const QString& path);
	QJsonArray extractJsonArray(const QJsonObject& data, const QString& path);
	QList<VideoInfo> parseVideoInfo(const QMap<int, QJsonObject>& responseMap, const QVariantMap& parser);

private:
	ModInfo m_modInfo;
	QSharedPointer<NetworkManager> m_networkManager;
	QScopedPointer<LoginManager> m_loginManager;
};