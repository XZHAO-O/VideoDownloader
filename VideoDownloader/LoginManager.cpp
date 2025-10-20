#include "LoginManager.h"

#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

#include "INetworkManager.h"
#include "LogSystem.h"
#include "CookieManageUtil.h"

LoginManager::LoginManager(const ModInfo& modInfo, QString modPath, QSharedPointer<INetworkManager> networkManager, QObject* parent)
	: QObject(parent)
	, m_modInfo(modInfo)
	, m_modPath(modPath)
	, m_networkManager(networkManager)
	, m_qrCodePollTimer(new QTimer(this))
	, m_isQRCodeLoginActive(false)
	, m_isLoggedIn(false)
{
	// 初始化二维码轮询定时器
	m_qrCodePollTimer->setInterval(3000); // 3秒轮询一次
	connect(m_qrCodePollTimer, &QTimer::timeout, this, &LoginManager::onQRCodePollTimeout);

	// 加载保存的Cookie
	loadSavedCookies();
}

QUrl LoginManager::startQRCodeLogin()
{
	QVariantMap qrConfig = getQRCodeConfig();
	if (!qrConfig.value("enabled", false).toBool()) {
		LOG_WARN("LoginManager", "QR code login is not enabled in configuration");
		emit loginFailed("二维码登录功能未启用");
		return QUrl();
	}

	if (m_isQRCodeLoginActive) {
		LOG_WARN("LoginManager", "QR code login is already active");
		return QUrl();
	}

	m_isQRCodeLoginActive = true;
	m_checkParams.clear();

	LOG_INFO("LoginManager", "Starting QR code login");
	return generateQRCode();
}

void LoginManager::stopQRCodeLogin()
{
	if (m_qrCodePollTimer->isActive()) {
		m_qrCodePollTimer->stop();
	}

	m_isQRCodeLoginActive = false;
	m_checkParams.clear();

	LOG_INFO("LoginManager", "QR code login stopped");
}

bool LoginManager::isQRCodeLoginActive() const
{
	return m_isQRCodeLoginActive;
}

bool LoginManager::loadSavedCookies()
{
	if (CookieManageUtil::loadCookies(m_modInfo.modId, m_modPath, m_isLoggedIn, m_cookie)) {
		LOG_INFO("LoginManager", "Loaded saved cookies, login status: %d", loginStatus);
		emit loginStateChanged(m_isLoggedIn);
		emit cookieUpdated(m_cookie);
		return true;
	}
	return false;
}

void LoginManager::clearCookies()
{
	if (CookieManageUtil::clearCookies(m_modInfo.modId, m_modPath)) {
		m_isLoggedIn = false;
		m_cookie.clear();
		LOG_INFO("LoginManager", "Cookies cleared");
		emit loginStateChanged(false);
		emit cookieUpdated(QVariantMap());
	}
}

bool LoginManager::hasSavedCookies() const
{
	return CookieManageUtil::hasCookies(m_modInfo.modId, m_modPath);
}

QVariantMap LoginManager::getQRCodeConfig() const
{
	return m_modInfo.getConfigValue("qrCode").toMap();
}

QUrl LoginManager::generateQRCode()
{
	try {
		QVariantMap qrConfig = getQRCodeConfig();
		QString generateUrl = qrConfig.value("generateUrl").toString();

		if (generateUrl.isEmpty()) {
			throw std::runtime_error("QR code generate URL not configured");
		}

		QVariantMap headers = m_modInfo.getRequestHeaders();

		// 发送请求生成二维码
		NetworkResponse response = m_networkManager->get(generateUrl, headers);

		if (!response.success) {
			throw std::runtime_error(response.errorString.toStdString());
		}

		QJsonDocument doc = QJsonDocument::fromJson(response.data);
		if (doc.isNull()) {
			throw std::runtime_error("Invalid JSON response");
		}

		QJsonObject rootObj = doc.object();
		if (rootObj["code"].toInt() != 0) {
			throw std::runtime_error(QString("API error: %1").arg(rootObj["message"].toString()).toStdString());
		}

		QJsonObject data = rootObj["data"].toObject();

		// 从配置中获取字段映射
		QString codeUrlPath = qrConfig.value("codeUrl").toString();
		m_checkParams = qrConfig.value("checkParams").toMap();

		// 提取二维码URL和key
		QUrl qrCodeUrl = QUrl(extractJsonValue(rootObj, codeUrlPath).toString());
		for (auto it = m_checkParams.begin(); it != m_checkParams.end(); ++it) {
			it.value() = extractJsonValue(rootObj, it.value().toString()).toString();
		}

		if (qrCodeUrl.isEmpty() || m_checkParams.isEmpty()) {
			throw std::runtime_error("Failed to extract QR code data from response");
		}

		LOG_INFO("LoginManager", "QR code generated");

		// 获取状态码配置
		QVariantMap statusCodes = qrConfig.value("statusCode").toMap();
		int waitingCode = statusCodes.value("waiting").toInt();
		emit loginStatusChanged("请使用哔哩哔哩APP扫描二维码", waitingCode);

		// 开始轮询状态
		m_qrCodePollTimer->start();

		return qrCodeUrl;
	}
	catch (const std::exception& e) {
		LOG_ERROR("LoginManager", "Failed to generate QR code: %s", e.what());
		emit loginFailed(QString("生成二维码失败: %1").arg(e.what()));
		m_isQRCodeLoginActive = false;
		return QUrl();
	}
}

void LoginManager::pollQRCodeStatus()
{
	if (m_checkParams.isEmpty()) {
		return;
	}

	try {
		QVariantMap qrConfig = getQRCodeConfig();
		QString checkUrl = qrConfig.value("checkUrl").toString();

		if (checkUrl.isEmpty()) {
			throw std::runtime_error("QR code check URL not configured");
		}

		QVariantMap headers = m_modInfo.getRequestHeaders();

		// 构建查询参数
		QUrl url(checkUrl);
		QUrlQuery query;

		for (auto it = m_checkParams.begin(); it != m_checkParams.end(); ++it)
		{
			query.addQueryItem(it.key(), it.value().toString());
		}

		url.setQuery(query);

		NetworkResponse response = m_networkManager->get(url.toString(), headers);

		if (!response.success) {
			throw std::runtime_error(response.errorString.toStdString());
		}

		QJsonDocument doc = QJsonDocument::fromJson(response.data);
		if (doc.isNull()) {
			throw std::runtime_error("Invalid JSON response");
		}

		QJsonObject rootObj = doc.object();
		if (rootObj["code"].toInt() != 0) {
			throw std::runtime_error(QString("API error: %1").arg(rootObj["message"].toString()).toStdString());
		}

		// 从配置中获取状态字段和状态码
		QString statusPath = qrConfig.value("status").toString();
		QVariantMap statusCodes = qrConfig.value("statusCode").toMap();

		int statusCode = extractJsonValue(rootObj, statusPath).toInt();
		QString message = rootObj["message"].toString();

		LOG_INFO("LoginManager", "QR code status: %d - %s", statusCode, message.toUtf8().constData());

		// 根据状态码处理不同情况
		if (statusCode == statusCodes.value("success").toInt())
		{
			// 登录成功
			handleQRCodeLoginSuccess(rootObj);
		}
		else if (statusCode == statusCodes.value("expired").toInt())
		{
			// 二维码过期
			emit loginStatusChanged("二维码已失效，请重新扫描", statusCode);
			stopQRCodeLogin();
		}
		else if (statusCode == statusCodes.value("unconfirmed").toInt())
		{
			// 已扫描未确认
			emit loginStatusChanged("扫码成功，请在手机上确认登录", statusCode);
		}
		else if (statusCode == statusCodes.value("waiting").toInt())
		{
			// 等待扫描
			emit loginStatusChanged("请使用哔哩哔哩APP扫描二维码", statusCode);
		}
		else
		{
			// 其他状态
			emit loginStatusChanged(message, statusCode);
		}

	}
	catch (const std::exception& e) {
		LOG_ERROR("LoginManager", "Failed to poll QR code status: %s", e.what());
		// 不停止轮询，继续尝试
	}
}

void LoginManager::handleQRCodeLoginSuccess(const QJsonObject& data)
{
	try {
		QVariantMap cookieConfig = getQRCodeConfig().value("cookie").toMap();
		QString url = extractJsonValue(data, cookieConfig.value("url").toString()).toString();

		LOG_INFO("LoginManager", "QR code login success");

		// 解析URL获取cookies
		QUrl loginUrl(url);
		QUrlQuery query(loginUrl.query());
		m_cookie.clear();
		for (auto it = cookieConfig.begin(); it != cookieConfig.end(); ++it)
		{
			if (it.key() != "url")
			{
				QString value = it.value().toString();
				m_cookie[it.key()] = value.isEmpty() ? query.queryItemValue(it.key()) : extractJsonValue(data, value).toString();
			}
		}

		// 停止轮询
		stopQRCodeLogin();

		// 保存登录状态和Cookie
		if (CookieManageUtil::saveCookies(m_modInfo.modId, m_modPath, true, m_cookie)) {
			m_isLoggedIn = true;
			LOG_INFO("LoginManager", "Login status and cookies saved successfully");
			emit loginStateChanged(true);
			emit cookieUpdated(m_cookie);
		}

		// 发出成功信号
		emit loginStatusChanged("登录成功", 0);
		emit loginSuccess(m_cookie);

		LOG_INFO("LoginManager", "QR code login completed successfully");

	}
	catch (const std::exception& e) {
		LOG_ERROR("LoginManager", "Failed to handle QR code login success: %s", e.what());
		emit loginFailed(QString("处理登录成功数据失败: %1").arg(e.what()));
	}
}

int LoginManager::getQRStatusCode(const QString& status)
{
	QVariantMap qrConfig = getQRCodeConfig();
	QVariantMap statusCodes = qrConfig.value("statusCode").toMap();

	if (status == "success") return statusCodes.value("success").toInt();
	if (status == "unconfirmed") return statusCodes.value("unconfirmed").toInt();
	if (status == "waiting") return statusCodes.value("waiting").toInt();
	if (status == "expired") return statusCodes.value("expired").toInt();

	return -1;
}

void LoginManager::onQRCodePollTimeout()
{
	if (m_isQRCodeLoginActive && !m_checkParams.isEmpty()) {
		pollQRCodeStatus();
	}
}

QVariant LoginManager::extractJsonValue(const QJsonObject& data, const QString& path)
{
	if (path.isEmpty()) {
		return QVariant();
	}

	QStringList keys = path.split('.');
	QJsonValue current = data;

	for (const QString& key : keys) {
		if (current.isObject()) {
			current = current.toObject().value(key);
		}
		else if (current.isArray()) {
			bool ok;
			int index = key.toInt(&ok);
			if (ok && index >= 0 && index < current.toArray().size()) {
				current = current.toArray().at(index);
			}
			else {
				return QVariant();
			}
		}
		else {
			return QVariant();
		}

		if (current.isUndefined()) {
			return QVariant();
		}
	}

	return current.toVariant();
}