#pragma once

#include <QVariantMap>

#include "ModInfo.h"

class INetworkManager;

class LoginManager : public QObject
{
	Q_OBJECT

public:
	explicit LoginManager(const ModInfo& modInfo, QString modPath, QSharedPointer<INetworkManager> networkManager, QObject* parent = nullptr);

	// 二维码登录功能
	QUrl startQRCodeLogin();
	void stopQRCodeLogin();
	bool isQRCodeLoginActive() const;

	// Cookie 管理
	bool loadSavedCookies();
	void clearCookies();
	bool hasSavedCookies() const;
	QVariantMap getCookie() const { return m_cookie; }
	bool isLoggedIn() const { return m_isLoggedIn; }

signals:
	// 登录状态信号
	void loginStatusChanged(const QString& status, int code);
	void loginSuccess(const QVariantMap& cookie);
	void loginFailed(const QString& error);

	// Cookie 更新信号
	void cookieUpdated(const QVariantMap& cookie);
	void loginStateChanged(bool isLoggedIn);

private slots:
	void onQRCodePollTimeout();

private:
	// 内部方法
	QVariantMap getQRCodeConfig() const;
	QUrl generateQRCode();
	void pollQRCodeStatus();
	void handleQRCodeLoginSuccess(const QJsonObject& data);
	int getQRStatusCode(const QString& status);
	QVariant extractJsonValue(const QJsonObject& data, const QString& path);

	// 二维码登录相关成员变量
	QTimer* m_qrCodePollTimer;
	QVariantMap m_checkParams;
	bool m_isQRCodeLoginActive;

	// 登录状态和Cookie
	bool m_isLoggedIn;
	QVariantMap m_cookie;

	ModInfo m_modInfo;
	QString m_modPath;
	QSharedPointer<INetworkManager> m_networkManager;
};