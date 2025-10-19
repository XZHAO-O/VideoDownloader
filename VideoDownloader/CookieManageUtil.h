#pragma once

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QCryptographicHash>
#include <QDebug>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDateTime>
#include <QVariantMap>

class CookieManageUtil
{
public:
	// 保存登录状态和Cookie（加密）
	static bool saveCookies(const QString& modId, const QString& configDir,
		bool loginStatus, const QVariantMap& cookieMap);

	// 加载并解密Cookie信息
	static bool loadCookies(const QString& modId, const QString& configDir,
		bool& loginStatus, QVariantMap& cookieMap);

	// 清除保存的Cookie文件
	static bool clearCookies(const QString& modId, const QString& configDir);

	// 检查是否存在Cookie文件
	static bool hasCookies(const QString& modId, const QString& configDir);

	// 获取Cookie文件路径
	static QString getCookieFilePath(const QString& modId, const QString& configDir);

private:
	// 加密数据
	static QByteArray encryptData(const QByteArray& data, const QString& modId);

	// 解密数据
	static QByteArray decryptData(const QByteArray& encryptedData, const QString& modId);

	// 生成加密密钥（基于modId和设备信息）
	static QByteArray generateEncryptionKey(const QString& modId);

	// 保存JSON数据到文件
	static bool saveJsonToFile(const QJsonObject& jsonObject, const QString& filePath);

	// 从文件加载JSON数据
	static QJsonObject loadJsonFromFile(const QString& filePath);

	// 构建Cookie文件目录
	static QString buildCookieDir(const QString& modId, const QString& configDir);

	// QVariantMap 与 QJsonObject 互相转换
	static QJsonObject variantMapToJsonObject(const QVariantMap& variantMap);
	static QVariantMap jsonObjectToVariantMap(const QJsonObject& jsonObject);
};