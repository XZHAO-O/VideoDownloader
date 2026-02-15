#include "ApplicationController.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

//#include "DatabaseManager.h"
#include "DownloadRecordService.h"
#include "DownloadVideoCoverService.h"

#include "DownloadEngine.h"
#include "ConfigManager.h"
#include "ConfigModManager.h"
#include "EventBus.h"
#include "NetworkManager.h"
#include "PlatformAggregatorService.h"
#include "sqlite_database.h"

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

		LOG_INFO("ApplicationController initialized successfully");
		emit initializationComplete();

		return true;

	}
	catch (const std::exception& e) {
		m_state = Error;
		QString error = QString("Initialization failed: %1").arg(e.what());
		LOG_ERROR(error);
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
	LOG_INFO("ApplicationController shuttingdown");

	cleanup();

	m_initialized = false;
	m_state = Uninitialized;

	emit shutdownComplete();
}

void ApplicationController::initializeCoreSystems()
{
	const QString appDataPath = QCoreApplication::applicationDirPath();

	// 初始化日志系统
	nexusdl::log::Logger::instance();

	// 初始化配置管理器
	m_configManager = QSharedPointer<ConfigManager>::create(appDataPath + "/config");

	//设置日志等级

	/*m_databaseManager = QSharedPointer<DatabaseManager>::create();
	m_databaseManager->initialize(appDataPath + "/database/download.db");

	m_downloadRecordService = QSharedPointer<DownloadRecordService>::create(m_databaseManager);
	m_downloadVideoCoverService = QSharedPointer<DownloadVideoCoverService>::create(m_databaseManager);*/

	nexusdl::database::SQLiteDatabase database{ "download.db" };

	LOG_INFO("Core systems initialized");
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

	LOG_INFO("All services initialized");
}

void ApplicationController::initializeMods()
{
	if (!m_modManager->initialize()) {
		throw std::runtime_error("Failed to initialize mod manager");
	}

	int modCount = m_modManager->getAllMods().size();
	LOG_INFO(QString("Loaded %1 mods").arg(modCount));
}

void ApplicationController::cleanup()
{
	// 逆序清理服务
	m_downloadEngine.clear();
	m_platformService.clear();
	m_modManager.clear();
	m_networkManager.clear();

	//m_downloadVideoCoverService.clear();
	//m_downloadRecordService.clear();
	//m_databaseManager.clear();

	m_configManager.clear();
}