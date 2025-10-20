#pragma once

#include <QObject>
#include <QSharedPointer>

#include "ApplicationState.h"

class LogSystem;
class EventBus;
class NetworkManager;
class DownloadManager;
class ConfigManager;
class ConfigModManager;
class DownloadOrchestrationService;
class PlatformAggregatorService;
class MediaProcessingService;
class DownloadRecordRepository;

class ApplicationController : public QObject
{
	Q_OBJECT

public:
	explicit ApplicationController(QObject* parent = nullptr);
	~ApplicationController();

	bool initialize();
	void shutdown();

	// 获取服务实例
	QSharedPointer<ConfigManager> getConfigManager() const { return m_configManager; }
	QSharedPointer<ConfigModManager> getConfigModManager() const { return m_modManager; }  // 修改返回类型
	QSharedPointer<DownloadManager> getDownloadManager() const { return m_downloadManager; }
	QSharedPointer<DownloadOrchestrationService> getDownloadService() const { return m_downloadService; }
	QSharedPointer<PlatformAggregatorService> getPlatformService() const { return m_platformService; }
	QSharedPointer<MediaProcessingService> getMediaService() const { return m_mediaService; }

	// 应用状态
	ApplicationState getState() const { return m_state; }
	bool isInitialized() const { return m_initialized; }

signals:
	void initializationComplete();
	void shutdownComplete();
	void errorOccurred(const QString& error);

private:
	void initializeCoreSystems();
	void initializeServices();
	void initializeMods();
	void cleanup();

	ApplicationState m_state = Uninitialized;
	bool m_initialized = false;

	// 核心系统
	QSharedPointer<ConfigManager> m_configManager;
	QSharedPointer<LogSystem> m_logSystem;
	QSharedPointer<EventBus> m_eventBus;

	// 服务
	QSharedPointer<ConfigModManager> m_modManager;
	QSharedPointer<NetworkManager> m_networkManager;
	QSharedPointer<MediaProcessingService> m_mediaService;
	QSharedPointer<DownloadOrchestrationService> m_downloadService;
	QSharedPointer<PlatformAggregatorService> m_platformService;
	QSharedPointer<DownloadManager> m_downloadManager;
	QSharedPointer<DownloadRecordRepository> m_recordRepository;
};