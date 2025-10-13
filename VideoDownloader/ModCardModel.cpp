#include "ModCardModel.h"
#include <QDir>
#include <QFileInfo>

ModCardModel::ModCardModel(QObject* parent)
	: QObject(parent)
{
}

ModCardModel::ModCardModel(const ModManager::ModInfo& modInfo, QObject* parent)
	: QObject(parent)
{
	fromModInfo(modInfo);
}

void ModCardModel::setModId(const QString& modId)
{
	if (m_modId != modId) {
		m_modId = modId;
		emit modIdChanged();
	}
}

void ModCardModel::setName(const QString& name)
{
	if (m_name != name) {
		m_name = name;
		emit nameChanged();
	}
}

void ModCardModel::setAuthor(const QString& author)
{
	if (m_author != author) {
		m_author = author;
		emit authorChanged();
	}
}

void ModCardModel::setVersion(const QString& version)
{
	if (m_version != version) {
		m_version = version;
		emit versionChanged();
	}
}

void ModCardModel::setSize(qint64 size)
{
	if (m_size != size) {
		m_size = size;
		emit sizeChanged();
	}
}

void ModCardModel::setDescription(const QString& description)
{
	if (m_description != description) {
		m_description = description;
		emit descriptionChanged();
	}
}

void ModCardModel::setIconPath(const QString& iconPath)
{
	if (m_iconPath != iconPath) {
		m_iconPath = iconPath;
		emit iconPathChanged();
	}
}

void ModCardModel::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		emit enabledChanged();
	}
}

void ModCardModel::setModPath(const QString& modPath)
{
	if (m_modPath != modPath) {
		m_modPath = modPath;
		emit modPathChanged();
	}
}

void ModCardModel::setPlatformId(const QString& platformId)
{
	if (m_platformId != platformId) {
		m_platformId = platformId;
		emit platformIdChanged();
	}
}

void ModCardModel::setIsVideoPlatformMod(bool isVideoPlatformMod)
{
	if (m_isVideoPlatformMod != isVideoPlatformMod) {
		m_isVideoPlatformMod = isVideoPlatformMod;
		emit isVideoPlatformModChanged();
	}
}

QString ModCardModel::formattedSize() const
{
	if (m_size <= 0) return "未知大小";

	const qint64 KB = 1024;
	const qint64 MB = KB * 1024;
	const qint64 GB = MB * 1024;

	if (m_size >= GB)
		return QString("%1 GB").arg(QString::number(m_size / static_cast<double>(GB), 'f', 2));
	else if (m_size >= MB)
		return QString("%1 MB").arg(QString::number(m_size / static_cast<double>(MB), 'f', 1));
	else if (m_size >= KB)
		return QString("%1 KB").arg(m_size / KB);
	else
		return QString("%1 B").arg(m_size);
}

QString ModCardModel::formattedStatus() const
{
	return m_enabled ? "已启用" : "已禁用";
}

bool ModCardModel::hasUpdate() const
{
	// 这里可以实现检查更新的逻辑
	// 暂时返回false
	return false;
}

void ModCardModel::fromModInfo(const ModManager::ModInfo& modInfo)
{
	m_modId = modInfo.modId;
	m_modPath = modInfo.modPath;
	m_enabled = modInfo.enabled;

	// 从元数据中提取信息
	if (!modInfo.metadata.isEmpty()) {
		m_name = modInfo.metadata.value("name").toString();
		m_author = modInfo.metadata.value("author").toString();
		m_version = modInfo.metadata.value("version").toString();
		m_description = modInfo.metadata.value("description").toString();
		m_platformId = modInfo.metadata.value("platformId").toString();

		// 图标路径
		QString iconFile = modInfo.metadata.value("icon").toString();
		if (!iconFile.isEmpty()) {
			m_iconPath = modInfo.modPath + "/" + iconFile;
		}
	}

	// 计算模组大小
	QDir modDir(modInfo.modPath);
	if (modDir.exists()) {
		qint64 totalSize = 0;
		QFileInfoList files = modDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
		for (const QFileInfo& file : files) {
			totalSize += file.size();
		}
		m_size = totalSize;
	}

	// 检查是否为视频平台模组
	m_isVideoPlatformMod = modInfo.isVideoPlatformMod();

	// 触发所有信号
	emit modIdChanged();
	emit nameChanged();
	emit authorChanged();
	emit versionChanged();
	emit sizeChanged();
	emit descriptionChanged();
	emit iconPathChanged();
	emit enabledChanged();
	emit modPathChanged();
	emit platformIdChanged();
	emit isVideoPlatformModChanged();
}