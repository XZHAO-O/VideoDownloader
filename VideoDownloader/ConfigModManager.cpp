#include "ConfigModManager.h"
#include "ConfigManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <algorithm>

ConfigModManager::ConfigModManager(QSharedPointer<ConfigManager> configManager,
	QSharedPointer<INetworkManager> networkManager,
	QObject* parent)
	: QObject(parent)
	, m_configManager(configManager)
	, m_networkManager(networkManager)
{
	// 使用现有的LogSystem，不需要spdlog
	initialize();
}

bool ConfigModManager::initialize()
{
	if (m_initialized) {
		return true;
	}

	try {
		LOG_INFO("ModManager", "Initializing ConfigModManager");

		// 发现并加载所有Mod
		discoverMods();

		m_initialized = true;
		LOG_INFO("ModManager", "ConfigModManager initialized successfully, loaded %d mods", m_mods.size());
		return true;

	}
	catch (const std::exception& e) {
		LOG_ERROR("ModManager", "Failed to initialize ConfigModManager: %s", e.what());
		return false;
	}
}

void ConfigModManager::shutdown()
{
	LOG_INFO("ModManager", "Shutting down ConfigModManager");

	// 清理所有平台实例
	m_platforms.clear();
	m_mods.clear();
	m_urlPatterns.clear();
	m_initialized = false;
}

void ConfigModManager::discoverMods()
{
	LOG_INFO("ModManager", "Discovering mods");

	QString modsDir = m_configManager->getValue("mods/directory", "mods").toString();
	QDir dir(modsDir);

	if (!dir.exists()) {
		LOG_INFO("ModManager", "Mods directory does not exist, creating: %s", modsDir.toUtf8().constData());
		dir.mkpath(".");
		return;
	}

	// 查找所有 mod.json 文件
	auto entries = dir.entryInfoList(QStringList() << "*", QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QFileInfo& entry : entries) {
		QString modDir = entry.absoluteFilePath();
		QString configPath = QDir(modDir).absoluteFilePath("mod.json");

		if (QFile::exists(configPath)) {
			loadMod(configPath);
		}
	}

	buildUrlPatterns();
	LOG_INFO("ModManager", "Discovered %d mods", m_mods.size());
	emit modsChanged();
}

bool ConfigModManager::loadMod(const QString& configPath)
{
	QFileInfo fileInfo(configPath);
	QString modDir = fileInfo.absolutePath();

	QFile file(configPath);
	if (!file.open(QIODevice::ReadOnly)) {
		LOG_ERROR("ModManager", "Failed to open mod config: %s", configPath.toUtf8().constData());
		return false;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (doc.isNull()) {
		LOG_ERROR("ModManager", "Invalid JSON in mod config: %s", configPath.toUtf8().constData());
		return false;
	}

	QJsonObject config = doc.object();
	if (!validateModConfig(config)) {
		LOG_ERROR("ModManager", "Invalid mod config: %s", configPath.toUtf8().constData());
		return false;
	}

	ModInfo modInfo = ModInfo::fromJson(config, modDir);
	if (!modInfo.isValid()) {
		LOG_ERROR("ModManager", "Invalid mod info: %s", configPath.toUtf8().constData());
		return false;
	}

	// 检查是否启用
	bool enabled = m_configManager->getValue(
		QString("mods/%1/enabled").arg(modInfo.modId),
		modInfo.enabled
	).toBool();

	modInfo.enabled = enabled;

	// 如果已存在，先卸载
	if (m_mods.contains(modInfo.modId)) {
		unloadMod(modInfo.modId);
	}

	m_mods[modInfo.modId] = modInfo;

	// 创建平台实例
	if (enabled) {
		auto platform = QSharedPointer<ConfigVideoPlatform>::create(modInfo, m_networkManager);
		m_platforms[modInfo.modId] = platform;
		LOG_INFO("ModManager", "Loaded mod: %s v%s",
			modInfo.name.toUtf8().constData(),
			modInfo.version.toUtf8().constData());
		emit modLoaded(modInfo);
	}
	else {
		LOG_INFO("ModManager", "Loaded disabled mod: %s v%s",
			modInfo.name.toUtf8().constData(),
			modInfo.version.toUtf8().constData());
	}

	return true;
}

bool ConfigModManager::validateModConfig(const QJsonObject& config)
{
	// 必填字段验证
	if (!config.contains("modId") || config["modId"].toString().isEmpty()) {
		return false;
	}

	if (!config.contains("name") || config["name"].toString().isEmpty()) {
		return false;
	}

	if (!config.contains("urlPatterns") || !config["urlPatterns"].isArray()) {
		return false;
	}

	// URL模式验证
	QJsonArray urlPatterns = config["urlPatterns"].toArray();
	if (urlPatterns.isEmpty()) {
		return false;
	}

	for (const QJsonValue& pattern : urlPatterns) {
		QRegularExpression regex(pattern.toString());
		if (!regex.isValid()) {
			return false;
		}
	}

	return true;
}

void ConfigModManager::buildUrlPatterns()
{
	m_urlPatterns.clear();

	for (const auto& modInfo : m_mods) {
		if (!modInfo.enabled) continue;

		for (const QString& pattern : modInfo.urlPatterns) {
			QRegularExpression regex(pattern);
			if (regex.isValid()) {
				// 使用 QPair 而不是直接使用 QRegularExpression 作为键
				m_urlPatterns.insert(regex.pattern(), modInfo.modId);
				LOG_DEBUG("ModManager", "Registered URL pattern: %s -> %s",
					pattern.toUtf8().constData(),
					modInfo.modId.toUtf8().constData());
			}
			else {
				LOG_ERROR("ModManager", "Invalid URL pattern: %s for mod %s",
					pattern.toUtf8().constData(),
					modInfo.modId.toUtf8().constData());
			}
		}
	}
}

QList<ModInfo> ConfigModManager::getAllMods() const
{
	return m_mods.values();
}

ModInfo ConfigModManager::getMod(const QString& modId) const
{
	return m_mods.value(modId);
}

bool ConfigModManager::enableMod(const QString& modId)
{
	if (!m_mods.contains(modId)) {
		return false;
	}

	ModInfo modInfo = m_mods[modId];
	if (modInfo.enabled) {
		return true; // 已经是启用状态
	}

	modInfo.enabled = true;
	m_mods[modId] = modInfo;

	// 创建平台实例
	auto platform = QSharedPointer<ConfigVideoPlatform>::create(modInfo, m_networkManager);
	m_platforms[modId] = platform;

	// 更新配置
	m_configManager->setValue(QString("mods/%1/enabled").arg(modId), true);

	buildUrlPatterns();

	LOG_INFO("ModManager", "Enabled mod: %s", modId.toUtf8().constData());
	emit modEnabled(modId);
	emit modsChanged();

	return true;
}

bool ConfigModManager::disableMod(const QString& modId)
{
	if (!m_mods.contains(modId)) {
		return false;
	}

	ModInfo modInfo = m_mods[modId];
	if (!modInfo.enabled) {
		return true; // 已经是禁用状态
	}

	modInfo.enabled = false;
	m_mods[modId] = modInfo;

	// 移除平台实例
	m_platforms.remove(modId);

	// 更新配置
	m_configManager->setValue(QString("mods/%1/enabled").arg(modId), false);

	buildUrlPatterns();

	LOG_INFO("ModManager", "Disabled mod: %s", modId.toUtf8().constData());
	emit modDisabled(modId);
	emit modsChanged();

	return true;
}

bool ConfigModManager::refreshMod(const QString& modId)
{
	if (!m_mods.contains(modId)) {
		return false;
	}

	ModInfo modInfo = m_mods[modId];
	QString configPath = QDir(modInfo.modPath).absoluteFilePath("mod.json");

	return loadMod(configPath);
}

QString ConfigModManager::findModForUrl(const QString& url) const
{
	// 按优先级查找匹配的mod
	QList<QPair<int, QString>> matchedMods;

	for (auto it = m_urlPatterns.constBegin(); it != m_urlPatterns.constEnd(); ++it) {
		QRegularExpression regex(it.key());
		if (regex.match(url).hasMatch()) {
			QString modId = it.value();
			int priority = m_mods.value(modId).priority;
			matchedMods.append(qMakePair(priority, modId));
		}
	}

	if (matchedMods.isEmpty()) {
		return QString();
	}

	// 按优先级排序（优先级高的在前）
	std::sort(matchedMods.begin(), matchedMods.end(),
		[](const QPair<int, QString>& a, const QPair<int, QString>& b) {
			return a.first > b.first;
		});

	return matchedMods.first().second;
}

QSharedPointer<ConfigVideoPlatform> ConfigModManager::getPlatformForUrl(const QString& url)
{
	QString modId = findModForUrl(url);
	if (modId.isEmpty()) {
		return nullptr;
	}

	return getPlatformForMod(modId);
}

QSharedPointer<ConfigVideoPlatform> ConfigModManager::getPlatformForMod(const QString& modId)
{
	return m_platforms.value(modId);
}

QList<QString> ConfigModManager::getAvailablePlatforms() const
{
	return m_platforms.keys();
}

VideoInfo ConfigModManager::getVideoInfo(const QString& url)
{
	auto platform = getPlatformForUrl(url);
	if (!platform) {
		throw std::runtime_error("No platform found for URL: " + url.toStdString());
	}

	return platform->getVideoInfo(url);
}

QFuture<QList<StreamInfo>> ConfigModManager::getVideoStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
		auto platform = getPlatformForMod(videoInfo.platformId);
		if (!platform) {
			throw std::runtime_error("Platform not available: " + videoInfo.platformId.toStdString());
		}

		return platform->getVideoStreams(videoInfo, request).result();
		});
}

QFuture<QList<StreamInfo>> ConfigModManager::getAudioStreams(const VideoInfo& videoInfo, const StreamRequest& request)
{
	return QtConcurrent::run([this, videoInfo, request]() -> QList<StreamInfo> {
		auto platform = getPlatformForMod(videoInfo.platformId);
		if (!platform) {
			throw std::runtime_error("Platform not available: " + videoInfo.platformId.toStdString());
		}

		return platform->getAudioStreams(videoInfo, request).result();
		});
}

QFuture<SearchResult> ConfigModManager::searchVideos(const QString& keyword, const QString& platformId, int page)
{
	return QtConcurrent::run([this, keyword, platformId, page]() -> SearchResult {
		SearchResult combinedResult;
		combinedResult.searchQuery = keyword;

		QList<QFuture<SearchResult>> futures;

		// 如果指定了平台，只搜索该平台
		if (!platformId.isEmpty()) {
			auto platform = getPlatformForMod(platformId);
			if (platform) {
				futures.append(platform->searchVideos(keyword, page));
			}
		}
		else {
			// 搜索所有启用的平台
			for (const auto& platform : m_platforms) {
				futures.append(platform->searchVideos(keyword, page));
			}
		}

		// 等待所有搜索完成
		for (auto& future : futures) {
			try {
				SearchResult result = future.result();
				combinedResult.items.append(result.items);
				combinedResult.totalResults += result.totalResults;
			}
			catch (const std::exception& e) {
				LOG_WARN("ModManager", "Search failed for one platform: %s", e.what());
				// 忽略单个平台的搜索失败，继续处理其他平台
			}
		}

		return combinedResult;
		});
}

// 添加缺失的方法实现
QList<QString> ConfigModManager::getLoadedMods() const
{
	return m_mods.keys();
}

// 修改 unloadMod 方法为 public
bool ConfigModManager::unloadMod(const QString& modId)
{
	if (!m_mods.contains(modId)) {
		m_lastError = "Mod not found";
		return false;
	}

	m_platforms.remove(modId);
	m_mods.remove(modId);

	LOG_INFO("ModManager", "Unloaded mod: %s", modId.toUtf8().constData());
	emit modUnloaded(modId);

	// 重新构建 URL 模式
	buildUrlPatterns();

	return true;
}