#pragma once

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVariant>

class ConfigManager : public QObject
{
	Q_OBJECT

public:
	enum ConfigScope {
		GlobalScope,
		UserScope,
		ModScope
	};

	explicit ConfigManager(const QString& configDir, QObject* parent = nullptr);

	// 基本配置操作
	void setValue(const QString& key, const QVariant& value, ConfigScope scope = UserScope);
	QVariant getValue(const QString& key, const QVariant& defaultValue = {}) const;

	// Mod 配置管理
	void setModConfig(const QString& modId, const QJsonObject& config);
	QJsonObject getModConfig(const QString& modId) const;

	// 配置验证
	bool registerSchema(const QString& schemaId, const QJsonObject& schema);
	bool validateConfig(const QString& schemaId, const QJsonObject& config) const;

	// 配置迁移
	bool migrateConfig(const QString& fromVersion, const QString& toVersion);

	// 配置持久化
	void save();
	void load();

signals:
	void configChanged(const QString& key, const QVariant& value);
	void modConfigChanged(const QString& modId, const QJsonObject& config);

private:
	QString m_configDir;
	QJsonObject m_globalConfig;
	QJsonObject m_userConfig;
	QMap<QString, QJsonObject> m_modConfigs;
	QMap<QString, QJsonObject> m_schemas;

	QString getGlobalConfigPath() const;
	QString getUserConfigPath() const;
	QString getModConfigPath(const QString& modId) const;
};