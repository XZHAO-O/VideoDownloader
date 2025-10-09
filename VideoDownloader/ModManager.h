#pragma once

#include "IMod.h"
#include "IVideoPlatformMod.h"
#include "INetworkManager.h"
#include "ConfigManager.h"
#include "LogSystem.h"
#include <QObject>
#include <QMap>
#include <QList>
#include <QPluginLoader>
#include <QFuture>
#include <memory>

class ModManager : public QObject
{
	Q_OBJECT

public:
	struct ModInfo {
		QString modId;
		QString modPath;
		QPluginLoader* loader = nullptr;
		QSharedPointer<IMod> instance;
		QJsonObject metadata;
		bool enabled = true;
		QDateTime loadTime;
		QString errorString;

		bool isValid() const { return instance != nullptr; }
		bool isVideoPlatformMod() const {
			return qobject_cast<IVideoPlatformMod*>(instance.get()) != nullptr;
		}
	};

	explicit ModManager(QSharedPointer<ConfigManager> configManager,
		QSharedPointer<INetworkManager> networkManager,
		QObject* parent = nullptr);
	~ModManager();

	// Mod管理
	bool loadMod(const QString& modPath);
	bool unloadMod(const QString& modId);
	bool reloadMod(const QString& modId);
	void discoverMods(const QString& modsDirectory = "");

	// Mod查询
	QList<QString> getLoadedMods() const;
	QSharedPointer<IMod> getMod(const QString& modId) const;
	ModInfo getModInfo(const QString& modId) const;

	// 视频平台Mod查询
	QList<QString> getVideoPlatformMods() const;
	QSharedPointer<IVideoPlatformMod> getVideoPlatformMod(const QString& platformId) const;
	QSharedPointer<IVideoPlatformMod> getVideoPlatformModForUrl(const QUrl& url) const;

	// Mod状态管理
	bool enableMod(const QString& modId);
	bool disableMod(const QString& modId);
	bool isModEnabled(const QString& modId) const;

	// 依赖管理
	bool checkDependencies(const QString& modId) const;
	QList<QString> getMissingDependencies(const QString& modId) const;
	QList<QString> getConflictingMods(const QString& modId) const;

	// 配置管理
	void setModConfig(const QString& modId, const QJsonObject& config);
	QJsonObject getModConfig(const QString& modId) const;

	// 系统状态
	bool isInitialized() const { return m_initialized; }
	QString getLastError() const { return m_lastError; }

public slots:
	bool initialize();
	void shutdown();

signals:
	void modLoaded(const QString& modId, const ModInfo& info);
	void modUnloaded(const QString& modId);
	void modEnabled(const QString& modId);
	void modDisabled(const QString& modId);
	void modError(const QString& modId, const QString& error);
	void allModsLoaded();

private:
	class Impl;
	QScopedPointer<Impl> d;
	bool m_initialized = false;
	QString m_lastError;
};