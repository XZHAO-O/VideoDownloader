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
	QList<VideoInfo> getVideoInfo(const QString& url);
	QUrl getVideoPlayUrl(StreamRequest& request);
	QFuture<QList<StreamInfo>> getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<QList<StreamInfo>> getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<SearchResult> searchVideos(const QString& keyword, int page = 1);
	QUrl parseVideoPlayUrl(const QJsonObject& data);

	// 二维码登录功能
	QUrl startQRCodeLogin();
	void stopQRCodeLogin();
	bool isQRCodeLoginActive() const;

	QString getModId() const { return m_modInfo.modId; }
	QString getName() const { return m_modInfo.name; }
	bool isEnabled() const { return m_modInfo.enabled; }
	bool matchesUrl(const QString& url) const;

signals:
	void videoInfoReceived(const VideoInfo& videoInfo);
	void streamsReceived(const QList<StreamInfo>& streams);
	void searchResultsReceived(const SearchResult& results);
	void errorOccurred(const QString& error);

	// 二维码登录相关信号
	void qrCodeGenerated(const QPixmap& qrCodePixmap, const QString& qrCodeKey);
	void qrCodeLoginStatusChanged(const QString& status, int code);
	void qrCodeLoginSuccess(const QVariantMap& authData);
	void qrCodeLoginFailed(const QString& error);

private slots:
	void onQRCodePollTimeout();

private:
	// 内部方法
	QList<StreamInfo> parseStreams(const QJsonObject& data, StreamType type);
	QString extractVideoId(const QString& url);
	QVariantMap getQualityParams(const QString& qualityName, StreamType type) const;
	QVariantMap buildRequestParams(const VideoInfo& videoInfo, const StreamRequest& request);
	QVariant extractJsonValue(const QJsonObject& data, const QString& path);
	QJsonArray extractJsonArray(const QJsonObject& data, const QString& path);
	QList<VideoInfo> parseVideoInfo(const QJsonObject& data);

	// 二维码登录相关方法
	QUrl generateQRCode();
	void pollQRCodeStatus();
	void handleQRCodeLoginSuccess(const QJsonObject& data);
	QVariantMap getQRCodeConfig() const;
	int getQRStatusCode(const QString& status);

	// 二维码登录相关成员变量
	QTimer* m_qrCodePollTimer;
	QString m_qrCodeKey;
	bool m_isQRCodeLoginActive;

	ModInfo m_modInfo;
	QSharedPointer<INetworkManager> m_networkManager;
};