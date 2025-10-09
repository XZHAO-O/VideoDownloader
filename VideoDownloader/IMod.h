#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QVersionNumber>

class IMod : public QObject
{
	Q_OBJECT

public:
	explicit IMod(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~IMod() = default;

	// Mod基本信息
	virtual QString modId() const = 0;
	virtual QString modName() const = 0;
	virtual QVersionNumber version() const = 0;
	virtual QString author() const = 0;
	virtual QString description() const = 0;

	// Mod依赖
	virtual QList<QString> dependencies() const = 0;
	virtual QList<QString> conflicts() const = 0;

	// Mod生命周期
	virtual bool initialize() = 0;
	virtual void shutdown() = 0;
	virtual bool isInitialized() const = 0;

	// 配置管理
	virtual QJsonObject defaultConfig() const = 0;
	virtual void setConfig(const QJsonObject& config) = 0;
	virtual QJsonObject getConfig() const = 0;

	// 状态查询
	virtual bool isEnabled() const = 0;
	virtual void setEnabled(bool enabled) = 0;

signals:
	void modInitialized(const QString& modId);
	void modShutdown(const QString& modId);
	void modError(const QString& modId, const QString& error);
	void configChanged(const QString& modId, const QJsonObject& config);
};

Q_DECLARE_INTERFACE(IMod, "com.videodownloader.IMod/1.0")