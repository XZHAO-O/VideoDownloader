#include "ConfigManager.h"

#include <QJsonDocument>
#include <QDir>

ConfigManager::ConfigManager(const QString& configDir, QObject* parent)
	: QObject(parent)
	, m_configDir(configDir)
{
	// 确保配置目录存在
	QDir dir(configDir);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	load();
}

void ConfigManager::setValue(const QString& key, const QVariant& value, ConfigScope scope)
{
	QJsonObject* targetConfig = nullptr;

	switch (scope) {
	case GlobalScope:
		targetConfig = &m_globalConfig;
		break;
	case UserScope:
		targetConfig = &m_userConfig;
		break;
	case ModScope:
		// Mod 配置需要特殊处理
		return;
	default:
		qWarning() << "Unsupported config scope for key:" << key;
		return;
	}

	if (targetConfig) {
		targetConfig->insert(key, QJsonValue::fromVariant(value));
		emit configChanged(key, value);
	}
}

QVariant ConfigManager::getValue(const QString& key, const QVariant& defaultValue) const
{
	// 先在用户配置中查找
	if (m_userConfig.contains(key)) {
		return m_userConfig.value(key).toVariant();
	}

	// 然后在全局配置中查找
	if (m_globalConfig.contains(key)) {
		return m_globalConfig.value(key).toVariant();
	}

	return defaultValue;
}

void ConfigManager::setModConfig(const QString& modId, const QJsonObject& config)
{
	m_modConfigs[modId] = config;
	emit modConfigChanged(modId, config);
}

QJsonObject ConfigManager::getModConfig(const QString& modId) const
{
	return m_modConfigs.value(modId, QJsonObject());
}

bool ConfigManager::registerSchema(const QString& schemaId, const QJsonObject& schema)
{
	if (schema.isEmpty()) {
		qWarning() << "Cannot register empty schema:" << schemaId;
		return false;
	}

	m_schemas[schemaId] = schema;
	return true;
}

bool ConfigManager::validateConfig(const QString& schemaId, const QJsonObject& config) const
{
	if (!m_schemas.contains(schemaId)) {
		qWarning() << "Schema not found:" << schemaId;
		return false;
	}

	const QJsonObject& schema = m_schemas.value(schemaId);

	// 简单的必填字段验证
	for (auto it = schema.begin(); it != schema.end(); ++it) {
		const QString& field = it.key();
		const QJsonObject& fieldSchema = it.value().toObject();

		if (fieldSchema.value("required").toBool() && !config.contains(field)) {
			qWarning() << "Required field missing:" << field;
			return false;
		}

		// 简单的类型验证
		if (config.contains(field)) {
			QJsonValue::Type expectedType = static_cast<QJsonValue::Type>(fieldSchema.value("type").toInt());
			if (expectedType != QJsonValue::Undefined && config.value(field).type() != expectedType) {
				qWarning() << "Type mismatch for field:" << field;
				return false;
			}
		}
	}

	return true;
}

bool ConfigManager::migrateConfig(const QString& fromVersion, const QString& toVersion)
{
	// 简单的配置迁移逻辑
	// 在实际项目中，这里会根据版本差异执行相应的迁移操作
	qInfo() << "Migrating config from" << fromVersion << "to" << toVersion;

	// 示例：如果从 1.0 迁移到 2.0
	if (fromVersion == "1.0" && toVersion == "2.0") {
		// 执行迁移操作
		// 例如重命名字段、转换数据类型等
	}

	return true;
}

void ConfigManager::save()
{
	// 保存全局配置
	QFile globalFile(getGlobalConfigPath());
	if (globalFile.open(QIODevice::WriteOnly)) {
		QJsonDocument doc(m_globalConfig);
		globalFile.write(doc.toJson(QJsonDocument::Indented));
		globalFile.close();
	}
	else {
		qWarning() << "Failed to save global config:" << globalFile.errorString();
	}

	// 保存用户配置
	QFile userFile(getUserConfigPath());
	if (userFile.open(QIODevice::WriteOnly)) {
		QJsonDocument doc(m_userConfig);
		userFile.write(doc.toJson(QJsonDocument::Indented));
		userFile.close();
	}
	else {
		qWarning() << "Failed to save user config:" << userFile.errorString();
	}

	// 保存Mod配置
	for (auto it = m_modConfigs.begin(); it != m_modConfigs.end(); ++it) {
		QFile modFile(getModConfigPath(it.key()));
		if (modFile.open(QIODevice::WriteOnly)) {
			QJsonDocument doc(it.value());
			modFile.write(doc.toJson(QJsonDocument::Indented));
			modFile.close();
		}
		else {
			qWarning() << "Failed to save mod config for" << it.key() << ":" << modFile.errorString();
		}
	}

	qDebug() << "Configuration saved successfully";
}

void ConfigManager::load()
{
	// 加载全局配置
	QFile globalFile(getGlobalConfigPath());
	if (globalFile.open(QIODevice::ReadOnly)) {
		QJsonDocument doc = QJsonDocument::fromJson(globalFile.readAll());
		m_globalConfig = doc.object();
		globalFile.close();
	}
	else {
		qDebug() << "No global config found, using defaults";
	}

	// 加载用户配置
	QFile userFile(getUserConfigPath());
	if (userFile.open(QIODevice::ReadOnly)) {
		QJsonDocument doc = QJsonDocument::fromJson(userFile.readAll());
		m_userConfig = doc.object();
		userFile.close();
	}
	else {
		qDebug() << "No user config found, using defaults";
	}

	// 加载Mod配置
	QDir modConfigDir(m_configDir + "/mods");
	if (modConfigDir.exists()) {
		QStringList modConfigFiles = modConfigDir.entryList(QStringList() << "*.json", QDir::Files);
		for (const QString& fileName : modConfigFiles) {
			QString modId = fileName.left(fileName.lastIndexOf('.'));
			QFile modFile(modConfigDir.absoluteFilePath(fileName));
			if (modFile.open(QIODevice::ReadOnly)) {
				QJsonDocument doc = QJsonDocument::fromJson(modFile.readAll());
				m_modConfigs[modId] = doc.object();
				modFile.close();
			}
		}
	}

	qDebug() << "Configuration loaded successfully";
}

QString ConfigManager::getGlobalConfigPath() const
{
	return m_configDir + "/global_config.json";
}

QString ConfigManager::getUserConfigPath() const
{
	return m_configDir + "/user_config.json";
}

QString ConfigManager::getModConfigPath(const QString& modId) const
{
	QDir modDir(m_configDir + "/mods");
	if (!modDir.exists()) {
		modDir.mkpath(".");
	}
	return modDir.absoluteFilePath(modId + ".json");
}