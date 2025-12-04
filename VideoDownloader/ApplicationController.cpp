#include "ApplicationController.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

#include "DownloadEngine.h"
#include "ConfigManager.h"
#include "ConfigModManager.h"
#include "LogSystem.h"
#include "EventBus.h"
#include "NetworkManager.h"
#include "PlatformAggregatorService.h"

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
	const QString appDataPath = QCoreApplication::applicationDirPath();

	// 初始化日志系统
	LogSystem::instance().initialize(appDataPath + "/logs", LogSystem::Info);

	// 初始化配置管理器
	m_configManager = QSharedPointer<ConfigManager>::create(appDataPath + "/config");

	LogSystem::instance().info("Core systems initialized", "Application");
}

void ApplicationController::initializeServices()
{
	// 初始化网络管理器
	m_networkManager = QSharedPointer<NetworkManager>::create(m_configManager);

	// 初始化Mod管理器 - 使用新的ConfigModManager
	m_modManager = QSharedPointer<ConfigModManager>::create(m_configManager, m_networkManager);

	// 初始化平台聚合服务
	m_platformService = QSharedPointer<PlatformAggregatorService>::create(m_modManager);

	// 初始化下载管理器
	m_downloadEngine = QSharedPointer<DownloadEngine>::create(m_configManager, m_networkManager);

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
	m_downloadEngine.clear();
	m_platformService.clear();
	m_modManager.clear();
	m_networkManager.clear();

	m_configManager.clear();

	LogSystem::instance().shutdown();
}