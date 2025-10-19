#include "CookieManageUtil.h"

bool CookieManageUtil::saveCookies(const QString& modId, const QString& configDir,
	bool loginStatus, const QVariantMap& cookieMap)
{
	try {
		// 准备要保存的数据
		QJsonObject cookieData;
		cookieData["modId"] = modId;
		cookieData["loginStatus"] = loginStatus;
		cookieData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
		cookieData["version"] = "1.0";

		// 将 QVariantMap 转换为 QJsonObject
		QJsonObject cookieJson = variantMapToJsonObject(cookieMap);

		// 将 Cookie JSON 对象转换为字符串并加密
		QJsonDocument cookieDoc(cookieJson);
		QByteArray cookieBytes = cookieDoc.toJson(QJsonDocument::Compact);
		QByteArray encryptedCookie = encryptData(cookieBytes, modId);

		cookieData["encryptedCookie"] = QString(encryptedCookie.toBase64());

		// 获取文件路径并保存
		QString filePath = getCookieFilePath(modId, configDir);
		if (saveJsonToFile(cookieData, filePath)) {
			qDebug() << "Cookies saved successfully for mod:" << modId;
			return true;
		}
		else {
			qWarning() << "Failed to save cookies for mod:" << modId;
			return false;
		}

	}
	catch (const std::exception& e) {
		qCritical() << "Exception while saving cookies for mod" << modId << ":" << e.what();
		return false;
	}
}

bool CookieManageUtil::loadCookies(const QString& modId, const QString& configDir,
	bool& loginStatus, QVariantMap& cookieMap)
{
	try {
		if (!hasCookies(modId, configDir)) {
			qDebug() << "No cookie file found for mod:" << modId;
			return false;
		}

		// 从文件加载数据
		QString filePath = getCookieFilePath(modId, configDir);
		QJsonObject cookieData = loadJsonFromFile(filePath);
		if (cookieData.isEmpty()) {
			qWarning() << "Failed to load cookie data for mod:" << modId;
			return false;
		}

		// 验证modId是否匹配
		if (cookieData["modId"].toString() != modId) {
			qWarning() << "Cookie file modId mismatch for mod:" << modId;
			return false;
		}

		// 获取登录状态
		loginStatus = cookieData["loginStatus"].toBool();

		// 解密Cookie数据
		QByteArray encryptedCookie = QByteArray::fromBase64(cookieData["encryptedCookie"].toString().toUtf8());
		QByteArray decryptedCookie = decryptData(encryptedCookie, modId);

		if (decryptedCookie.isEmpty()) {
			qWarning() << "Failed to decrypt cookies for mod:" << modId;
			return false;
		}

		// 将解密后的数据解析为 JSON 对象，然后转换为 QVariantMap
		QJsonDocument cookieDoc = QJsonDocument::fromJson(decryptedCookie);
		if (cookieDoc.isNull()) {
			qWarning() << "Failed to parse decrypted cookie data as JSON for mod:" << modId;
			return false;
		}

		cookieMap = jsonObjectToVariantMap(cookieDoc.object());

		qDebug() << "Cookies loaded successfully for mod:" << modId;
		return true;

	}
	catch (const std::exception& e) {
		qCritical() << "Exception while loading cookies for mod" << modId << ":" << e.what();
		return false;
	}
}

bool CookieManageUtil::clearCookies(const QString& modId, const QString& configDir)
{
	QString filePath = getCookieFilePath(modId, configDir);
	QFile cookieFile(filePath);
	if (cookieFile.exists()) {
		if (cookieFile.remove()) {
			qDebug() << "Cookies cleared for mod:" << modId;
			return true;
		}
		else {
			qWarning() << "Failed to remove cookie file for mod:" << modId;
			return false;
		}
	}
	qDebug() << "No cookie file to clear for mod:" << modId;
	return true;
}

bool CookieManageUtil::hasCookies(const QString& modId, const QString& configDir)
{
	QString filePath = getCookieFilePath(modId, configDir);
	return QFile::exists(filePath);
}

QString CookieManageUtil::getCookieFilePath(const QString& modId, const QString& configDir)
{
	return buildCookieDir(modId, configDir) + "/cookies";
}

QString CookieManageUtil::buildCookieDir(const QString& modId, const QString& configDir)
{
	QDir modDir(configDir + "/" + modId);
	if (!modDir.exists()) {
		modDir.mkpath(".");
	}
	return modDir.absolutePath();
}

QByteArray CookieManageUtil::encryptData(const QByteArray& data, const QString& modId)
{
	// 简单的XOR加密
	QByteArray key = generateEncryptionKey(modId);
	QByteArray encrypted = data;

	for (int i = 0; i < encrypted.size(); ++i) {
		encrypted[i] = encrypted[i] ^ key[i % key.size()];
	}

	return encrypted;
}

QByteArray CookieManageUtil::decryptData(const QByteArray& encryptedData, const QString& modId)
{
	// XOR解密（加密和解密使用相同的操作）
	return encryptData(encryptedData, modId);
}

QByteArray CookieManageUtil::generateEncryptionKey(const QString& modId)
{
	// 基于modId和设备特定信息生成加密密钥
	QString baseKey = modId +
		QCoreApplication::applicationName() +
		QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

	// 使用SHA256生成固定长度的密钥
	QCryptographicHash hash(QCryptographicHash::Sha256);
	hash.addData(baseKey.toUtf8());

	return hash.result();
}

bool CookieManageUtil::saveJsonToFile(const QJsonObject& jsonObject, const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open cookie file for writing:" << file.errorString();
		return false;
	}

	QJsonDocument doc(jsonObject);
	qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));
	file.close();

	return (bytesWritten > 0);
}

QJsonObject CookieManageUtil::loadJsonFromFile(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open cookie file for reading:" << file.errorString();
		return QJsonObject();
	}

	QByteArray fileData = file.readAll();
	file.close();

	QJsonDocument doc = QJsonDocument::fromJson(fileData);
	if (doc.isNull()) {
		qWarning() << "Failed to parse cookie file as JSON";
		return QJsonObject();
	}

	return doc.object();
}

QJsonObject CookieManageUtil::variantMapToJsonObject(const QVariantMap& variantMap)
{
	QJsonObject jsonObject;
	for (auto it = variantMap.begin(); it != variantMap.end(); ++it) {
		jsonObject[it.key()] = QJsonValue::fromVariant(it.value());
	}
	return jsonObject;
}

QVariantMap CookieManageUtil::jsonObjectToVariantMap(const QJsonObject& jsonObject)
{
	QVariantMap variantMap;
	for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it) {
		variantMap[it.key()] = it.value().toVariant();
	}
	return variantMap;
}