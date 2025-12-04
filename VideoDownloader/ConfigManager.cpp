#include "ConfigManager.h"

#include <QJsonDocument>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>

// 定义默认配置常量
const QMap<QString, QVariant> ConfigManager::DEFAULT_CONFIG = {
	// 常规设置
	{"app/autoStart", false},
	{"app/checkForUpdates", false},
	{"ui/minimizeToTray", false},
	{"ui/language", "简体中文"},
	{"ui/theme", "dark"},
	{"ui/startupPage", "home"},
	{"ui/showTrayIcon", true},
	{"ui/closeToTray", false},

	{"mods/directory", "mods"},

	// 下载设置
	{"download/defaultVideoQuality", "最高质量"},
	{"download/defaultAudioQuality", "最高质量"},
	{"download/defaultFormat", "视频+音频(合并)"},
	{"download/maxConcurrentDownloads", 3},
	{"download/autoMerge", true},
	{"download/autoDeleteTempFiles", true},

	// 网络设置
	{"network/timeout", 30000},
	{"network/retryCount", 3},
	{"network/userAgent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"},
	{"network/proxy/enabled", false},
	{"network/proxy/type", "HTTP"},
	{"network/proxy/host", ""},
	{"network/proxy/port", ""},
	{"network/proxy/username", ""},
	{"network/proxy/password", ""},

	// 高级设置
	{"log/level", "Info"},
	{"log/maxSize", 10485760}, // 10MB
	{"log/maxFiles", 5}
};

ConfigManager::ConfigManager(const QString& configDir, QObject* parent)
	: QObject(parent)
{
	// 确保配置目录存在
	QDir dir(configDir);
	if (!dir.exists())
	{
		dir.mkpath(".");
	}

	m_configFile.setFileName(configDir + "/config.json");
	ensureConfigFileExists();
	load();
}

void ConfigManager::ensureConfigFileExists()
{
	// 如果配置文件不存在，创建并写入默认值
	if (!m_configFile.exists())
	{
		// 创建文件
		if (m_configFile.open(QIODevice::WriteOnly))
		{
			// 设置动态默认值（路径相关）
			QJsonObject defaultConfig = getDefaultConfig();

			// 下载路径
			QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
			if (defaultDownloadPath.isEmpty()) {
				defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
				QDir dir(defaultDownloadPath);
				if (!dir.exists()) {
					dir.mkpath(".");
				}
			}
			defaultConfig["download/defaultSavePath"] = defaultDownloadPath;

			// 日志路径
			QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
			if (defaultLogPath.isEmpty()) {
				defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";
			}
			else {
				defaultLogPath += "/logs";
			}
			// 确保日志目录存在
			QDir logDir(defaultLogPath);
			if (!logDir.exists()) {
				logDir.mkpath(".");
			}
			defaultConfig["log/path"] = defaultLogPath;

			// 写入文件
			QJsonDocument doc(defaultConfig);
			m_configFile.write(doc.toJson(QJsonDocument::Indented));
			m_configFile.close();

			qDebug() << "Config file created with default values";
		}
		else
		{
			qWarning() << "Failed to create config file:" << m_configFile.errorString();
		}
	}
}

void ConfigManager::setValue(const QString& key, const QVariant& value)
{
	m_config.insert(key, QJsonValue::fromVariant(value));
	emit configChanged(key, value);
}

QVariant ConfigManager::getValue(const QString& key) const
{
	auto it = m_config.find(key);
	if (it != m_config.end())
	{
		return it.value().toVariant();
	}

	// 如果在当前配置中找不到，返回默认值
	QVariant defaultValue = getDefaultValue(key);

	// 注意：这里不直接修改配置文件，因为这是const方法
	// 如果需要自动写入，可以在非const方法中调用
	return defaultValue;
}

QVariant ConfigManager::getDefaultValue(const QString& key) const
{
	// 从默认配置中查找
	if (DEFAULT_CONFIG.contains(key)) {
		return DEFAULT_CONFIG[key];
	}

	// 特殊处理路径相关的默认值
	if (key == "download/defaultSavePath") {
		QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		if (defaultDownloadPath.isEmpty()) {
			defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
			QDir dir(defaultDownloadPath);
			if (!dir.exists()) {
				dir.mkpath(".");
			}
		}
		return defaultDownloadPath;
	}

	if (key == "log/path") {
		QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		if (defaultLogPath.isEmpty()) {
			defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";
		}
		else {
			defaultLogPath += "/logs";
		}
		// 确保日志目录存在
		QDir logDir(defaultLogPath);
		if (!logDir.exists()) {
			logDir.mkpath(".");
		}
		return defaultLogPath;
	}

	// 如果默认配置中也没有，返回空值
	return QVariant();
}

void ConfigManager::save()
{
	// 确保所有默认配置项都存在
	for (auto it = DEFAULT_CONFIG.begin(); it != DEFAULT_CONFIG.end(); ++it) {
		if (!m_config.contains(it.key())) {
			m_config.insert(it.key(), QJsonValue::fromVariant(it.value()));
		}
	}

	// 确保动态路径配置存在
	if (!m_config.contains("download/defaultSavePath")) {
		QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		if (defaultDownloadPath.isEmpty()) {
			defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
		}
		m_config.insert("download/defaultSavePath", defaultDownloadPath);
	}

	if (!m_config.contains("log/path")) {
		QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		if (defaultLogPath.isEmpty()) {
			defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";
		}
		else {
			defaultLogPath += "/logs";
		}
		m_config.insert("log/path", defaultLogPath);
	}

	// 保存配置
	if (m_configFile.open(QIODevice::WriteOnly))
	{
		QJsonDocument doc(m_config);
		m_configFile.write(doc.toJson(QJsonDocument::Indented));
		m_configFile.close();
	}
	else
	{
		qWarning() << "Failed to save config:" << m_configFile.errorString();
	}

	qDebug() << "Configuration saved successfully";
}

void ConfigManager::load()
{
	// 加载配置
	if (m_configFile.open(QIODevice::ReadOnly))
	{
		QJsonDocument doc = QJsonDocument::fromJson(m_configFile.readAll());
		m_config = doc.object();
		m_configFile.close();

		// 确保所有默认配置项都存在
		bool configUpdated = false;
		for (auto it = DEFAULT_CONFIG.begin(); it != DEFAULT_CONFIG.end(); ++it) {
			if (!m_config.contains(it.key())) {
				m_config.insert(it.key(), QJsonValue::fromVariant(it.value()));
				configUpdated = true;
			}
		}

		// 确保动态路径配置存在
		if (!m_config.contains("download/defaultSavePath")) {
			QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
			if (defaultDownloadPath.isEmpty()) {
				defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
				QDir dir(defaultDownloadPath);
				if (!dir.exists()) {
					dir.mkpath(".");
				}
			}
			m_config.insert("download/defaultSavePath", defaultDownloadPath);
			configUpdated = true;
		}

		if (!m_config.contains("log/path")) {
			QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
			if (defaultLogPath.isEmpty()) {
				defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";
			}
			else {
				defaultLogPath += "/logs";
			}
			// 确保日志目录存在
			QDir logDir(defaultLogPath);
			if (!logDir.exists()) {
				logDir.mkpath(".");
			}
			m_config.insert("log/path", defaultLogPath);
			configUpdated = true;
		}

		// 如果配置有更新，保存回文件
		if (configUpdated) {
			save();
		}
	}
	else
	{
		qDebug() << "No config found, using defaults";
		// 如果文件无法打开（可能不存在），使用默认配置
		m_config = getDefaultConfig();

		// 添加动态路径
		QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
		if (defaultDownloadPath.isEmpty()) {
			defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
			QDir dir(defaultDownloadPath);
			if (!dir.exists()) {
				dir.mkpath(".");
			}
		}
		m_config.insert("download/defaultSavePath", defaultDownloadPath);

		QString defaultLogPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		if (defaultLogPath.isEmpty()) {
			defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";
		}
		else {
			defaultLogPath += "/logs";
		}
		QDir logDir(defaultLogPath);
		if (!logDir.exists()) {
			logDir.mkpath(".");
		}
		m_config.insert("log/path", defaultLogPath);
	}

	qDebug() << "Configuration loaded successfully";
}

QJsonObject ConfigManager::getDefaultConfig()
{
	QJsonObject defaultConfig;

	// 将默认配置转换为QJsonObject
	for (auto it = DEFAULT_CONFIG.begin(); it != DEFAULT_CONFIG.end(); ++it) {
		defaultConfig.insert(it.key(), QJsonValue::fromVariant(it.value()));
	}

	return defaultConfig;
}