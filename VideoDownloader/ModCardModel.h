#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QVersionNumber>
#include "ModInfo.h"

class ModCardModel : public QObject
{
	Q_OBJECT

		Q_PROPERTY(QString modId READ modId WRITE setModId NOTIFY modIdChanged)
		Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
		Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
		Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY versionChanged)
		Q_PROPERTY(qint64 size READ size WRITE setSize NOTIFY sizeChanged)
		Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
		Q_PROPERTY(QString iconPath READ iconPath WRITE setIconPath NOTIFY iconPathChanged)
		Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
		Q_PROPERTY(QString modPath READ modPath WRITE setModPath NOTIFY modPathChanged)
		Q_PROPERTY(QString platformId READ platformId WRITE setPlatformId NOTIFY platformIdChanged)
		Q_PROPERTY(bool isVideoPlatformMod READ isVideoPlatformMod WRITE setIsVideoPlatformMod NOTIFY isVideoPlatformModChanged)

public:
	explicit ModCardModel(QObject* parent = nullptr);
	explicit ModCardModel(const ModInfo& modInfo, QObject* parent = nullptr);

	// Getters
	QString modId() const { return m_modId; }
	QString name() const { return m_name; }
	QString author() const { return m_author; }
	QString version() const { return m_version; }
	qint64 size() const { return m_size; }
	QString description() const { return m_description; }
	QString iconPath() const { return m_iconPath; }
	bool enabled() const { return m_enabled; }
	QString modPath() const { return m_modPath; }
	QString platformId() const { return m_platformId; }
	bool isVideoPlatformMod() const { return m_isVideoPlatformMod; }

	// Setters
	void setModId(const QString& modId);
	void setName(const QString& name);
	void setAuthor(const QString& author);
	void setVersion(const QString& version);
	void setSize(qint64 size);
	void setDescription(const QString& description);
	void setIconPath(const QString& iconPath);
	void setEnabled(bool enabled);
	void setModPath(const QString& modPath);
	void setPlatformId(const QString& platformId);
	void setIsVideoPlatformMod(bool isVideoPlatformMod);

	// 工具方法
	QString formattedSize() const;
	QString formattedStatus() const;
	bool hasUpdate() const;

	// 从ModInfo转换
	void fromModInfo(const ModInfo& modInfo);

signals:
	void modIdChanged();
	void nameChanged();
	void authorChanged();
	void versionChanged();
	void sizeChanged();
	void descriptionChanged();
	void iconPathChanged();
	void enabledChanged();
	void modPathChanged();
	void platformIdChanged();
	void isVideoPlatformModChanged();

private:
	QString m_modId;
	QString m_name;
	QString m_author;
	QString m_version;
	qint64 m_size = 0;
	QString m_description;
	QString m_iconPath;
	bool m_enabled = false;
	QString m_modPath;
	QString m_platformId;
	bool m_isVideoPlatformMod = false;
};