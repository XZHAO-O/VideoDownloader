#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "INetworkManager.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QNetworkProxy>
#include <QAuthenticator>
#include <QFuture>
#include <QMutex>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDateTime>
#include <memory>

class ConfigManager;

class NetworkManager : public QObject, public INetworkManager
{
	Q_OBJECT

public:
	explicit NetworkManager(QSharedPointer<ConfigManager> configManager,
		QObject* parent = nullptr);
	~NetworkManager();

	// INetworkManager 接口实现
	QFuture<NetworkResponse> get(const QString& url,
		const QVariantMap& headers = {}) override;
	QFuture<NetworkResponse> post(const QString& url,
		const QVariantMap& data = {},
		const QVariantMap& headers = {}) override;
	QFuture<NetworkResponse> post(const QString& url,
		const QByteArray& data,
		const QVariantMap& headers = {}) override;

	void setProxy(const NetworkProxy& proxy) override;
	void setTimeout(int milliseconds) override;
	void setRetryCount(int count) override;

	// Cookie 管理
	void setCookies(const QString& domain, const QList<QNetworkCookie>& cookies);
	QList<QNetworkCookie> getCookies(const QString& domain) const;
	void clearCookies();

	// 用户代理设置
	void setUserAgent(const QString& userAgent);
	QString userAgent() const;

private slots:
	void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
	void onProxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator);
	void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

private:
	struct RequestContext {
		QString id;
		QNetworkRequest request;
		QByteArray data;
		int retryCount = 0;
		int maxRetries = 3;
		QFutureInterface<NetworkResponse> futureInterface;
		QTimer* timeoutTimer = nullptr;
		bool finished = false;
	};

	void handleReply(QNetworkReply* reply, std::shared_ptr<RequestContext> context);
	void retryRequest(std::shared_ptr<RequestContext> context);
	void completeRequest(std::shared_ptr<RequestContext> context, const NetworkResponse& response);

	QString generateRequestId() const;

	QNetworkAccessManager* m_networkManager;
	QSharedPointer<ConfigManager> m_configManager;
	QNetworkCookieJar* m_cookieJar;

	QMap<QString, std::shared_ptr<RequestContext>> m_activeRequests;
	QMutex m_requestsMutex;

	NetworkProxy m_proxy;
	int m_timeoutMs = 30000;
	int m_defaultRetryCount = 3;
	QString m_userAgent;

	static const int MAX_CONCURRENT_REQUESTS = 10;
};

#endif // NETWORKMANAGER_H