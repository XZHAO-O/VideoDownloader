#include "ApplicationController.h"

#include <QDir>
#include <QStandardPaths>

#include "DownloadManager.h"
#include "ConfigManager.h"
#include "ConfigModManager.h"
#include "LogSystem.h"
#include "EventBus.h"
#include "NetworkManager.h"
#include "DownloadOrchestrationService.h"
#include "PlatformAggregatorService.h"
#include "MediaProcessingService.h"
#include "DownloadRecordRepository.h"

ApplicationController::ApplicationController(QObject* parent)
	: QObject(parent)
{
	initialize();
}

ApplicationController::~ApplicationController()
{
	shutdown();
}

bool ApplicationController::initialize()
{
	if (m_initialized) {
		return true;
	}

	m_state = Initializing;

	try {
		// 初始化核心系统
		initializeCoreSystems();

		// 初始化服务
		initializeServices();

		// 初始化Mods
		initializeMods();

		m_state = Running;
		m_initialized = true;

		LogSystem::instance().info("ApplicationController initialized successfully", "Application");
		emit initializationComplete();

		return true;

	}
	catch (const std::exception& e) {
		m_state = Error;
		QString error = QString("Initialization failed: %1").arg(e.what());
		LogSystem::instance().error(error, "Application");
		emit errorOccurred(error);
		return false;
	}
}

void ApplicationController::shutdown()
{
	if (!m_initialized) {
		return;
	}

	m_state = ShuttingDown;
	LogSystem::instance().info("ApplicationController shuttingdown", "Application");

	cleanup();

	m_initialized = false;
	m_state = Uninitialized;

	emit shutdownComplete();
}

void ApplicationController::initializeCoreSystems()
{
	// 设置应用数据目录
	QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	QDir dataDir(appDataDir);
	if (!dataDir.exists()) {
		dataDir.mkpath(".");
	}

	// 初始化日志系统
	QString logDir = appDataDir + "/logs";
	LogSystem::instance().initialize(logDir, LogSystem::Info);

	// 初始化配置管理器
	m_configManager = QSharedPointer<ConfigManager>::create(appDataDir);

	// 初始化事件总线
	m_eventBus = EventBus::instance();

	LogSystem::instance().info("Core systems initialized", "Application");
}

void ApplicationController::initializeServices()
{
	// 初始化网络管理器
	m_networkManager = QSharedPointer<NetworkManager>::create(m_configManager);

	// 初始化媒体处理服务
	m_mediaService = QSharedPointer<MediaProcessingService>::create(m_configManager);

	// 初始化Mod管理器 - 使用新的ConfigModManager
	m_modManager = QSharedPointer<ConfigModManager>::create(m_configManager, m_networkManager);

	// 初始化下载记录仓库
	m_recordRepository = QSharedPointer<DownloadRecordRepository>::create(
		m_configManager->getValue("storage/downloadRecordsPath",
			QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VideoDownloader/records").toString());

	// 初始化平台聚合服务
	m_platformService = QSharedPointer<PlatformAggregatorService>::create(m_modManager);

	// 初始化下载编排服务
	m_downloadService = QSharedPointer<DownloadOrchestrationService>::create(
		m_modManager, m_networkManager, m_mediaService, m_recordRepository);

	// 初始化下载管理器
	m_downloadManager = QSharedPointer<DownloadManager>::create();

	LogSystem::instance().info("All services initialized", "Application");
}

void ApplicationController::initializeMods()
{
	if (!m_modManager->initialize()) {
		throw std::runtime_error("Failed to initialize mod manager");
	}

	int modCount = m_modManager->getAllMods().size();
	LogSystem::instance().info(QString("Loaded %1 mods").arg(modCount), "Application");
}

void ApplicationController::cleanup()
{
	// 逆序清理服务
	m_downloadManager.clear();
	m_downloadService.clear();
	m_platformService.clear();
	m_recordRepository.clear();
	m_modManager.clear();
	m_mediaService.clear();
	m_networkManager.clear();

	// 清理核心系统
	m_eventBus.clear();
	m_configManager.clear();

	LogSystem::instance().shutdown();
}