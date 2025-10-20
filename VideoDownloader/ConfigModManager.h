#pragma once

#include <QFuture>

#include "ModInfo.h"
#include "SearchResult.h"

class ConfigVideoPlatform;
class INetworkManager;
class ConfigManager;

class ConfigModManager : public QObject
{
	Q_OBJECT

public:
	explicit ConfigModManager(QSharedPointer<ConfigManager> configManager,
		QSharedPointer<INetworkManager> networkManager,
		QObject* parent = nullptr);

	bool initialize();
	void shutdown();

	// Mod 发现和管理
	void discoverMods();
	QList<ModInfo> getAllMods() const;
	ModInfo getMod(const QString& modId) const;
	bool enableMod(const QString& modId);
	bool disableMod(const QString& modId);
	bool refreshMod(const QString& modId);
	bool unloadMod(const QString& modId);  // 添加这个方法

	// URL 匹配和平台获取
	QString findModForUrl(const QString& url) const;
	QSharedPointer<ConfigVideoPlatform> getPlatformForUrl(const QString& url);
	QSharedPointer<ConfigVideoPlatform> getPlatformForMod(const QString& modId);
	QList<QString> getAvailablePlatforms() const;

	// 获取已加载的 Mod ID 列表
	QList<QString> getLoadedMods() const;  // 添加这个方法

	// 平台操作
	QList<VideoInfo> getVideoInfo(const QString& url);
	QFuture<QList<StreamInfo>> getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<QList<StreamInfo>> getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request);
	QFuture<SearchResult> searchVideos(const QString& keyword, const QString& platformId = "", int page = 1);

	// 系统状态
	bool isInitialized() const { return m_initialized; }
	QString getLastError() const { return m_lastError; }

signals:
	void modsChanged();
	void modLoaded(const ModInfo& info);
	void modUnloaded(const QString& modId);
	void modEnabled(const QString& modId);
	void modDisabled(const QString& modId);

private:
	bool loadMod(const QString& configPath);
	bool validateModConfig(const QJsonObject& config);
	void buildUrlPatterns();

	QSharedPointer<ConfigManager> m_configManager;
	QSharedPointer<INetworkManager> m_networkManager;
	QMap<QString, ModInfo> m_mods;
	QMap<QString, QSharedPointer<ConfigVideoPlatform>> m_platforms;
	QMap<QString, QString> m_urlPatterns; // 正则表达式字符串 -> modId
	bool m_initialized = false;
	QString m_lastError;  // 添加错误信息成员
};