#pragma once

#include "IVideoPlatformMod.h"
#include "INetworkManager.h"
#include <QJsonObject>
#include <QFuture>
#include <QFutureWatcher>
#include <memory>

class BaseVideoPlatformMod : public IVideoPlatformMod
{
	Q_OBJECT
		Q_INTERFACES(IVideoPlatformMod)

public:
	explicit BaseVideoPlatformMod(QObject* parent = nullptr);
	virtual ~BaseVideoPlatformMod();

	// 工具方法
	void setMetadata(const QJsonObject& metadata);
	void setNetworkManager(QSharedPointer<INetworkManager> networkManager);

	// IMod 接口实现
	QString modId() const override { return m_metadata["modId"].toString(); }
	QString modName() const override { return m_metadata["name"].toString(); }
	QVersionNumber version() const override {
		return QVersionNumber::fromString(m_metadata["version"].toString());
	}
	QString author() const override { return m_metadata["author"].toString(); }
	QString description() const override { return m_metadata["description"].toString(); }

	QList<QString> dependencies() const override;
	QList<QString> conflicts() const override;

	bool initialize() override;
	void shutdown() override;
	bool isInitialized() const override { return m_initialized; }

	QJsonObject defaultConfig() const override;
	void setConfig(const QJsonObject& config) override;
	QJsonObject getConfig() const override { return m_config; }

	bool isEnabled() const override { return m_enabled; }
	void setEnabled(bool enabled) override;

	// IVideoPlatformMod 接口实现
	QString platformId() const override { return m_metadata["platformId"].toString(); }
	QString platformName() const override { return m_metadata["platformName"].toString(); }
	QString platformIcon() const override { return m_metadata["icon"].toString(); }
	QList<QString> supportedDomains() const override;

	// 需要子类实现的纯虚函数
	virtual QFuture<VideoInfo> getVideoInfo(const QUrl& videoUrl) override = 0;
	virtual QFuture<QList<StreamInfo>> getVideoStreams(const QString& videoId,
		const VideoQuality& quality) override = 0;
	virtual QFuture<QList<StreamInfo>> getAudioStreams(const QString& videoId,
		const AudioQuality& quality) override = 0;
	virtual QFuture<SearchResult> searchVideos(const QString& query,
		int page = 1,
		int resultsPerPage = 20) override = 0;
	virtual QFuture<SearchResult> searchVideosByChannel(const QString& channelId,
		int page = 1,
		int resultsPerPage = 20) override = 0;
	virtual QFuture<bool> downloadVideo(const VideoDownloadRequest& request) override = 0;
	virtual bool supportsFormat(DownloadFormat format) const override = 0;
	virtual QList<VideoQuality> getSupportedVideoQualities() const override = 0;
	virtual QList<AudioQuality> getSupportedAudioQualities() const override = 0;
	virtual bool isValidUrl(const QUrl& url) const override = 0;
	virtual bool isAvailable() const override = 0;
	virtual QString getLastError() const override { return m_lastError; }
	virtual QFuture<QList<VideoInfo>> getChannelVideos(const QString& channelId,
		int page = 1,
		int resultsPerPage = 20) override = 0;
	virtual QFuture<QList<VideoInfo>> getPlaylistVideos(const QString& playlistId,
		int page = 1,
		int resultsPerPage = 20) override = 0;

protected:
	// 工具方法

	QSharedPointer<INetworkManager> networkManager() const { return m_networkManager; }

	void setLastError(const QString& error) { m_lastError = error; }
	QJsonObject mergeConfig(const QJsonObject& userConfig) const;

	// 网络请求辅助方法
	QFuture<NetworkResponse> httpGet(const QString& url, const QVariantMap& headers = {});
	QFuture<NetworkResponse> httpPost(const QString& url, const QVariantMap& data = {},
		const QVariantMap& headers = {});
	QFuture<NetworkResponse> httpPost(const QString& url, const QByteArray& data,
		const QVariantMap& headers = {});

	// 配置辅助方法
	QString getConfigValue(const QString& key, const QString& defaultValue = "") const;
	int getConfigValue(const QString& key, int defaultValue = 0) const;
	bool getConfigValue(const QString& key, bool defaultValue = false) const;

private:
	QJsonObject m_metadata;
	QJsonObject m_config;
	QSharedPointer<INetworkManager> m_networkManager;
	bool m_initialized = false;
	bool m_enabled = true;
	QString m_lastError;
};