#include "BaseVideoPlatformMod.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QJsonArray>

BaseVideoPlatformMod::BaseVideoPlatformMod(QObject* parent)
	: IVideoPlatformMod(parent)
{
}

BaseVideoPlatformMod::~BaseVideoPlatformMod()
{
	shutdown();
}

QList<QString> BaseVideoPlatformMod::dependencies() const
{
	QList<QString> deps;
	if (m_metadata.contains("dependencies")) {
		QJsonArray depArray = m_metadata["dependencies"].toArray();
		for (const auto& dep : depArray) {
			deps.append(dep.toString());
		}
	}
	return deps;
}

QList<QString> BaseVideoPlatformMod::conflicts() const
{
	QList<QString> conflicts;
	if (m_metadata.contains("conflicts")) {
		QJsonArray conflictArray = m_metadata["conflicts"].toArray();
		for (const auto& conflict : conflictArray) {
			conflicts.append(conflict.toString());
		}
	}
	return conflicts;
}

bool BaseVideoPlatformMod::initialize()
{
	if (m_initialized) {
		return true;
	}

	if (!m_networkManager) {
		m_lastError = "Network manager not set";
		return false;
	}

	// 验证元数据
	if (m_metadata.isEmpty()) {
		m_lastError = "Mod metadata not set";
		return false;
	}

	if (!m_metadata.contains("modId") || m_metadata["modId"].toString().isEmpty()) {
		m_lastError = "Invalid mod ID";
		return false;
	}

	m_initialized = true;
	emit modInitialized(modId());

	return true;
}

void BaseVideoPlatformMod::shutdown()
{
	if (m_initialized) {
		m_initialized = false;
		emit modShutdown(modId());
	}
}

QJsonObject BaseVideoPlatformMod::defaultConfig() const
{
	QJsonObject defaultConfig;
	if (m_metadata.contains("defaultConfig")) {
		defaultConfig = m_metadata["defaultConfig"].toObject();
	}
	return defaultConfig;
}

void BaseVideoPlatformMod::setConfig(const QJsonObject& config)
{
	QJsonObject oldConfig = m_config;
	m_config = mergeConfig(config);

	if (oldConfig != m_config) {
		emit configChanged(modId(), m_config);
	}
}

QList<QString> BaseVideoPlatformMod::supportedDomains() const
{
	QList<QString> domains;
	if (m_metadata.contains("supportedDomains")) {
		QJsonArray domainArray = m_metadata["supportedDomains"].toArray();
		for (const auto& domain : domainArray) {
			domains.append(domain.toString());
		}
	}
	return domains;
}

void BaseVideoPlatformMod::setMetadata(const QJsonObject& metadata)
{
	m_metadata = metadata;
}

void BaseVideoPlatformMod::setNetworkManager(QSharedPointer<INetworkManager> networkManager)
{
	m_networkManager = networkManager;
}

QJsonObject BaseVideoPlatformMod::mergeConfig(const QJsonObject& userConfig) const
{
	QJsonObject merged = defaultConfig();

	for (auto it = userConfig.begin(); it != userConfig.end(); ++it) {
		merged[it.key()] = it.value();
	}

	return merged;
}

QFuture<NetworkResponse> BaseVideoPlatformMod::httpGet(const QString& url, const QVariantMap& headers)
{
	if (!m_networkManager) {
		QFutureInterface<NetworkResponse> future;
		future.reportFinished();
		return future.future();
	}
	return m_networkManager->get(url, headers);
}

QFuture<NetworkResponse> BaseVideoPlatformMod::httpPost(const QString& url, const QVariantMap& data, const QVariantMap& headers)
{
	if (!m_networkManager) {
		QFutureInterface<NetworkResponse> future;
		future.reportFinished();
		return future.future();
	}
	return m_networkManager->post(url, data, headers);
}

QFuture<NetworkResponse> BaseVideoPlatformMod::httpPost(const QString& url, const QByteArray& data, const QVariantMap& headers)
{
	if (!m_networkManager) {
		QFutureInterface<NetworkResponse> future;
		future.reportFinished();
		return future.future();
	}
	return m_networkManager->post(url, data, headers);
}

QString BaseVideoPlatformMod::getConfigValue(const QString& key, const QString& defaultValue) const
{
	return m_config.value(key).toString(defaultValue);
}

int BaseVideoPlatformMod::getConfigValue(const QString& key, int defaultValue) const
{
	return m_config.value(key).toInt(defaultValue);
}

bool BaseVideoPlatformMod::getConfigValue(const QString& key, bool defaultValue) const
{
	return m_config.value(key).toBool(defaultValue);
}

void BaseVideoPlatformMod::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		if (enabled) {
			initialize();
		}
		else {
			shutdown();
		}
	}
}