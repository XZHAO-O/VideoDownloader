#pragma once

#include <QJsonObject>
#include <QFile>

class ConfigManager : public QObject
{
	Q_OBJECT

public:
	explicit ConfigManager(const QString& configDir, QObject* parent = nullptr);

	void setValue(const QString& key, const QVariant& value);
	QVariant getValue(const QString& key) const;

	void save();
	void load();

	// 获取默认设置
	static QJsonObject getDefaultConfig();

signals:
	void configChanged(const QString& key, const QVariant& value);

private:
	void ensureConfigFileExists();
	QVariant getDefaultValue(const QString& key) const;

	QFile m_configFile;
	QJsonObject m_config;

	// 默认配置常量
	static const QMap<QString, QVariant> DEFAULT_CONFIG;
};