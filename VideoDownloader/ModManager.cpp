#include "ModManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include "BaseVideoPlatformMod.h"

class ModManager::Impl
{
public:
	QSharedPointer<ConfigManager> configManager;
	QSharedPointer<INetworkManager> networkManager;
	QMap<QString, ModInfo> mods;
	QString modsDirectory;

	Impl(QSharedPointer<ConfigManager> config, QSharedPointer<INetworkManager> netMgr)
		: configManager(config)
		, networkManager(netMgr)
	{
	}

	bool loadModMetadata(const QString& modPath, QJsonObject& metadata)
	{
		QFile metadataFile(modPath + "/mod.json");
		if (!metadataFile.exists()) {
			LogSystem::instance().error("Mod metadata file not found: " + modPath, "ModManager");
			return false;
		}

		if (!metadataFile.open(QIODevice::ReadOnly)) {
			LogSystem::instance().error("Failed to open mod metadata: " + metadataFile.errorString(), "ModManager");
			return false;
		}

		QJsonDocument doc = QJsonDocument::fromJson(metadataFile.readAll());
		if (doc.isNull()) {
			LogSystem::instance().error("Invalid mod metadata JSON: " + modPath, "ModManager");
			return false;
		}

		metadata = doc.object();
		return true;
	}

	QString findModLibrary(const QString& modPath)
	{
		QDir modDir(modPath);

		// 查找平台特定的库文件
		#ifdef Q_OS_WIN
		QString libraryName = "mod.dll";
		#else
		QString libraryName = "mod.so";
		#endif

		if (modDir.exists(libraryName)) {
			return modDir.absoluteFilePath(libraryName);
		}

		// 查找任何共享库文件
		QStringList filters;
		#ifdef Q_OS_WIN
		filters << "*.dll";
		#else
		filters << "*.so";
		#endif

		QStringList libraries = modDir.entryList(filters, QDir::Files);
		if (!libraries.isEmpty()) {
			return modDir.absoluteFilePath(libraries.first());
		}

		return "";
	}

	bool validateModMetadata(const QJsonObject& metadata)
	{
		if (!metadata.contains("modId") || metadata["modId"].toString().isEmpty()) {
			LogSystem::instance().error("Mod metadata missing modId", "ModManager");
			return false;
		}

		if (!metadata.contains("name") || metadata["name"].toString().isEmpty()) {
			LogSystem::instance().error("Mod metadata missing name", "ModManager");
			return false;
		}

		if (!metadata.contains("version")) {
			LogSystem::instance().error("Mod metadata missing version", "ModManager");
			return false;
		}

		// 验证版本格式
		QVersionNumber version = QVersionNumber::fromString(metadata["version"].toString());
		if (version.isNull()) {
			LogSystem::instance().error("Invalid mod version: " + metadata["version"].toString(), "ModManager");
			return false;
		}

		return true;
	}

	bool checkDependencies(const QString& modId, const QJsonObject& metadata)
	{
		if (!metadata.contains("dependencies")) {
			return true;
		}

		QJsonArray dependencies = metadata["dependencies"].toArray();
		for (const auto& dep : dependencies) {
			QString depId = dep.toString();
			if (!mods.contains(depId) || !mods[depId].enabled) {
				LogSystem::instance().error("Missing dependency: " + depId + " for mod " + modId, "ModManager");
				return false;
			}
		}

		return true;
	}

	bool checkConflicts(const QString& modId, const QJsonObject& metadata)
	{
		if (!metadata.contains("conflicts")) {
			return true;
		}

		QJsonArray conflicts = metadata["conflicts"].toArray();
		for (const auto& conflict : conflicts) {
			QString conflictId = conflict.toString();
			if (mods.contains(conflictId) && mods[conflictId].enabled) {
				LogSystem::instance().error("Mod conflict: " + modId + " conflicts with " + conflictId, "ModManager");
				return false;
			}
		}

		return true;
	}
};

ModManager::ModManager(QSharedPointer<ConfigManager> configManager,
	QSharedPointer<INetworkManager> networkManager,
	QObject* parent)
	: QObject(parent)
	, d(new Impl(configManager, networkManager))
{
}

ModManager::~ModManager()
{
	shutdown();
}

bool ModManager::initialize()
{
	if (m_initialized) {
		return true;
	}

	// 设置Mods目录
	QString appDir = QCoreApplication::applicationDirPath();
	d->modsDirectory = appDir + "/mods";

	// 发现并加载Mods
	discoverMods(d->modsDirectory);

	m_initialized = true;
	LogSystem::instance().info("ModManager initialized with " + QString::number(d->mods.size()) + " mods", "ModManager");

	return true;
}

void ModManager::shutdown()
{
	if (!m_initialized) {
		return;
	}

	// 卸载所有Mods
	for (const QString& modId : d->mods.keys()) {
		unloadMod(modId);
	}

	m_initialized = false;
	LogSystem::instance().info("ModManager shutdown", "ModManager");
}

bool ModManager::loadMod(const QString& modPath)
{
	try {
		LogSystem::instance().debug("Loading mod from: " + modPath, "ModManager");

		// 加载元数据
		QJsonObject metadata;
		if (!d->loadModMetadata(modPath, metadata)) {
			m_lastError = "Failed to load mod metadata";
			return false;
		}

		QString modId = metadata["modId"].toString();

		// 验证元数据
		if (!d->validateModMetadata(metadata)) {
			m_lastError = "Invalid mod metadata";
			return false;
		}

		// 检查是否已加载
		if (d->mods.contains(modId)) {
			LogSystem::instance().warning("Mod already loaded: " + modId, "ModManager");
			return true;
		}

		// 检查依赖和冲突
		if (!d->checkDependencies(modId, metadata) || !d->checkConflicts(modId, metadata)) {
			m_lastError = "Dependency or conflict check failed";
			return false;
		}

		// 查找库文件
		QString libraryPath = d->findModLibrary(modPath);
		if (libraryPath.isEmpty()) {
			m_lastError = "No mod library found";
			return false;
		}

		// 加载插件
		QPluginLoader* loader = new QPluginLoader(libraryPath);
		if (!loader->load()) {
			m_lastError = QString("Failed to load mod library: %1").arg(loader->errorString());
			delete loader;
			return false;
		}

		// 获取Mod实例
		IMod* modInstance = qobject_cast<IMod*>(loader->instance());
		if (!modInstance) {
			m_lastError = "Invalid mod instance - does not implement IMod interface";
			loader->unload();
			delete loader;
			return false;
		}

		// 设置网络管理器（如果是视频平台Mod）
		if (auto videoMod = qobject_cast<IVideoPlatformMod*>(modInstance)) {
			if (auto baseMod = qobject_cast<BaseVideoPlatformMod*>(videoMod)) {
				baseMod->setNetworkManager(QSharedPointer<INetworkManager>(d->networkManager.data()));
				baseMod->setMetadata(metadata);
			}
		}

		// 加载配置
		QJsonObject modConfig = d->configManager->getModConfig(modId);

		// 初始化Mod
		if (!modInstance->initialize()) {
			m_lastError = "Mod initialization failed";
			loader->unload();
			delete loader;
			return false;
		}

		// 设置配置
		modInstance->setConfig(modConfig);

		// 保存Mod信息
		ModInfo info;
		info.modId = modId;
		info.modPath = modPath;
		info.loader = loader;
		info.instance = QSharedPointer<IMod>(modInstance);
		info.metadata = metadata;
		info.enabled = true;
		info.loadTime = QDateTime::currentDateTime();

		d->mods[modId] = info;

		// 连接信号
		QObject::connect(modInstance, &IMod::modError, this, [this, modId](const QString&, const QString& error) {
			emit modError(modId, error);
			});

		QObject::connect(modInstance, &IMod::configChanged, this, [this, modId](const QString&, const QJsonObject& config) {
			d->configManager->setModConfig(modId, config);
			});

		LogSystem::instance().info("Mod loaded successfully: " + modId, "ModManager");
		emit modLoaded(modId, info);

		return true;

	}
	catch (const std::exception& e) {
		m_lastError = QString("Exception while loading mod: %1").arg(e.what());
		LogSystem::instance().error("Exception loading mod: " + modPath + " - " + e.what(), "ModManager");
		return false;
	}
}

bool ModManager::unloadMod(const QString& modId)
{
	if (!d->mods.contains(modId)) {
		m_lastError = "Mod not found";
		return false;
	}

	ModInfo info = d->mods[modId];

	try {
		// 关闭Mod
		info.instance->shutdown();

		// 卸载库
		if (info.loader) {
			info.loader->unload();
			delete info.loader;
		}

		d->mods.remove(modId);

		LogSystem::instance().info("Mod unloaded: " + modId, "ModManager");
		emit modUnloaded(modId);

		return true;

	}
	catch (const std::exception& e) {
		m_lastError = QString("Exception while unloading mod: %1").arg(e.what());
		LogSystem::instance().error("Exception unloading mod: " + modId + " - " + e.what(), "ModManager");
		return false;
	}
}

bool ModManager::reloadMod(const QString& modId)
{
	if (!d->mods.contains(modId)) {
		return false;
	}

	ModInfo info = d->mods[modId];
	QString modPath = info.modPath;

	if (!unloadMod(modId)) {
		return false;
	}

	return loadMod(modPath);
}

void ModManager::discoverMods(const QString& modsDirectory)
{
	QDir modsDir(modsDirectory);
	if (!modsDir.exists()) {
		LogSystem::instance().info("Mods directory does not exist, creating: " + modsDirectory, "ModManager");
		modsDir.mkpath(".");
		return;
	}

	LogSystem::instance().info("Discovering mods in: " + modsDirectory, "ModManager");

	int loadedCount = 0;
	for (const QString& entry : modsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		QString modPath = modsDir.absoluteFilePath(entry);

		if (loadMod(modPath)) {
			loadedCount++;
		}
		else {
			LogSystem::instance().error("Failed to load mod: " + modPath + " - " + m_lastError, "ModManager");
		}
	}

	LogSystem::instance().info("Discovered and loaded " + QString::number(loadedCount) + " mods", "ModManager");
	emit allModsLoaded();
}

QList<QString> ModManager::getLoadedMods() const
{
	return d->mods.keys();
}

QSharedPointer<IMod> ModManager::getMod(const QString& modId) const
{
	return d->mods.value(modId).instance;
}

ModManager::ModInfo ModManager::getModInfo(const QString& modId) const
{
	return d->mods.value(modId);
}

QList<QString> ModManager::getVideoPlatformMods() const
{
	QList<QString> platformMods;
	for (const auto& info : d->mods) {
		if (info.isVideoPlatformMod() && info.enabled) {
			platformMods.append(info.modId);
		}
	}
	return platformMods;
}

QSharedPointer<IVideoPlatformMod> ModManager::getVideoPlatformMod(const QString& platformId) const
{
	for (const auto& info : d->mods) {
		if (info.isVideoPlatformMod() && info.enabled) {
			if (auto videoMod = qobject_cast<IVideoPlatformMod*>(info.instance.get())) {
				if (videoMod->platformId() == platformId) {
					return QSharedPointer<IVideoPlatformMod>(videoMod);
				}
			}
		}
	}
	return nullptr;
}

QSharedPointer<IVideoPlatformMod> ModManager::getVideoPlatformModForUrl(const QUrl& url) const
{
	for (const auto& info : d->mods) {
		if (info.isVideoPlatformMod() && info.enabled) {
			if (auto videoMod = qobject_cast<IVideoPlatformMod*>(info.instance.get())) {
				if (videoMod->isValidUrl(url)) {
					return QSharedPointer<IVideoPlatformMod>(videoMod);
				}
			}
		}
	}
	return nullptr;
}

bool ModManager::enableMod(const QString& modId)
{
	if (!d->mods.contains(modId)) {
		return false;
	}

	ModInfo& info = d->mods[modId];
	if (!info.enabled) {
		info.enabled = true;
		info.instance->setEnabled(true);
		emit modEnabled(modId);
	}

	return true;
}

bool ModManager::disableMod(const QString& modId)
{
	if (!d->mods.contains(modId)) {
		return false;
	}

	ModInfo& info = d->mods[modId];
	if (info.enabled) {
		info.enabled = false;
		info.instance->setEnabled(false);
		emit modDisabled(modId);
	}

	return true;
}

bool ModManager::isModEnabled(const QString& modId) const
{
	return d->mods.value(modId).enabled;
}

bool ModManager::checkDependencies(const QString& modId) const
{
	if (!d->mods.contains(modId)) {
		return false;
	}

	return d->checkDependencies(modId, d->mods[modId].metadata);
}

QList<QString> ModManager::getMissingDependencies(const QString& modId) const
{
	QList<QString> missing;

	if (!d->mods.contains(modId)) {
		return missing;
	}

	QJsonObject metadata = d->mods[modId].metadata;
	if (metadata.contains("dependencies")) {
		QJsonArray dependencies = metadata["dependencies"].toArray();
		for (const auto& dep : dependencies) {
			QString depId = dep.toString();
			if (!d->mods.contains(depId) || !d->mods[depId].enabled) {
				missing.append(depId);
			}
		}
	}

	return missing;
}

QList<QString> ModManager::getConflictingMods(const QString& modId) const
{
	QList<QString> conflicts;

	if (!d->mods.contains(modId)) {
		return conflicts;
	}

	QJsonObject metadata = d->mods[modId].metadata;
	if (metadata.contains("conflicts")) {
		QJsonArray conflictArray = metadata["conflicts"].toArray();
		for (const auto& conflict : conflictArray) {
			QString conflictId = conflict.toString();
			if (d->mods.contains(conflictId) && d->mods[conflictId].enabled) {
				conflicts.append(conflictId);
			}
		}
	}

	return conflicts;
}

void ModManager::setModConfig(const QString& modId, const QJsonObject& config)
{
	if (d->mods.contains(modId)) {
		d->mods[modId].instance->setConfig(config);
	}
	d->configManager->setModConfig(modId, config);
}

QJsonObject ModManager::getModConfig(const QString& modId) const
{
	return d->configManager->getModConfig(modId);
}