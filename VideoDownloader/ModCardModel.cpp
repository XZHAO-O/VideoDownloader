#include "ModCardModel.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

ModCardModel::ModCardModel(QObject* parent)
	: QObject(parent)
{
}

ModCardModel::ModCardModel(const ModInfo& modInfo, QObject* parent)
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

void ModCardModel::fromModInfo(const ModInfo& modInfo)
{
	setModId(modInfo.modId);
	setName(modInfo.name);
	setAuthor(modInfo.author);
	setVersion(modInfo.version);
	setDescription(modInfo.description);
	setEnabled(modInfo.enabled);
	setModPath(modInfo.modPath);
	setPlatformId(modInfo.modId);
	setIsVideoPlatformMod(true);

	// 设置图标路径
	QString iconPath = modInfo.modPath + "/icon.png";
	if (QFile::exists(iconPath)) {
		setIconPath(iconPath);
	}
	//else {
	//	// 使用默认图标或平台特定图标
	//	setIconPath(":/icons/icon.png"); // 假设有默认图标资源
	//	// 或者根据平台设置不同的默认图标
	//	if (modInfo.modId == "bilibili") {
	//		setIconPath(":/icons/bilibili.png");
	//	}
	//}

	// 计算目录大小（可选）
	QDir modDir(modInfo.modPath);
	qint64 size = 0;
	QFileInfoList files = modDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	for (const QFileInfo& file : files) {
		size += file.size();
	}
	setSize(size);
}

//void ModCardModel::fromModInfo(const ModInfo& modInfo)
//{
//	m_modId = modInfo.modId;
//	m_modPath = modInfo.modPath;
//	m_enabled = modInfo.enabled;
//
//	// 直接从 ModInfo 结构体中获取信息
//	m_name = modInfo.name;
//	m_author = modInfo.author;
//	m_version = modInfo.version;
//	m_description = modInfo.description;
//
//	// 设置平台ID为modId（因为配置式Mod系统中，modId就是平台ID）
//	m_platformId = modInfo.modId;
//
//	// 图标路径处理
//	QString iconFile = modInfo.getConfigValue("icon").toString();
//	if (iconFile.isEmpty()) {
//		// 尝试常见的图标文件名
//		QStringList possibleIcons = { "icon.png", "icon.jpg", "mod.png", "logo.png" };
//		for (const QString& iconName : possibleIcons) {
//			QString iconPath = modInfo.modPath + "/" + iconName;
//			if (QFile::exists(iconPath)) {
//				iconFile = iconName;
//				break;
//			}
//		}
//	}
//
//	if (!iconFile.isEmpty()) {
//		m_iconPath = modInfo.modPath + "/" + iconFile;
//	}
//
//	// 计算模组大小
//	QDir modDir(modInfo.modPath);
//	if (modDir.exists()) {
//		qint64 totalSize = 0;
//		QFileInfoList files = modDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden);
//		for (const QFileInfo& file : files) {
//			totalSize += file.size();
//		}
//		m_size = totalSize;
//	}
//
//	// 检查是否为视频平台模组 - 配置式Mod系统默认都是视频平台模组
//	// 或者通过检查是否有urlPatterns来判断
//	m_isVideoPlatformMod = !modInfo.urlPatterns.isEmpty();
//
//	// 触发所有信号
//	emit modIdChanged();
//	emit nameChanged();
//	emit authorChanged();
//	emit versionChanged();
//	emit sizeChanged();
//	emit descriptionChanged();
//	emit iconPathChanged();
//	emit enabledChanged();
//	emit modPathChanged();
//	emit platformIdChanged();
//	emit isVideoPlatformModChanged();
//}