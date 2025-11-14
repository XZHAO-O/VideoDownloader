#pragma once

#include <QObject>
#include <QFuture>
#include <QSslError>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QMutex>
#include <QSharedPointer>

class QNetworkAccessManager;
class QNetworkProxy;
class QNetworkCookieJar;
class QAuthenticator;
class QTimer;
class ConfigManager;
class DownloadContext;

struct NetworkResponse
{
	bool success;
	QByteArray data;
	QString errorString;
};

struct NetworkReplyHeader
{
	bool success = true;
	QHash<QByteArray, QByteArray> headers;
	QString errorString;

	QByteArray getAcceptRanges() const
	{
		auto it = headers.find("accept-ranges");
		if (it != headers.end())
			return it.value();
		return QByteArray();
	}

	qint64 getContentLength() const
	{
		QString contentLength;
		auto it = headers.find("content-length");
		if (it != headers.end())
			contentLength = it.value();
		return contentLength.isEmpty() ? 0 : contentLength.toLongLong();
	}
};

struct NetworkProxy
{
	bool enabled = false;
	QString type; // "http", "socks5"
	QString host;
	int port = 0;
	QString username;
	QString password;
};

class NetworkManager : public QObject
{
	Q_OBJECT

public:
	explicit NetworkManager(QSharedPointer<ConfigManager> configManager,
		QObject* parent = nullptr);
	~NetworkManager();

	QNetworkRequest setRequest(const QUrl& url, const QVariantMap& headers = {});

	// 网络请求方法
	NetworkReplyHeader getReplyWithLoop(const QUrl& url, const QVariantMap& headers = {});
	NetworkResponse get(const QString& url,
		const QVariantMap& headers = {});
	NetworkResponse getWithLoop(const QUrl& url,
		const QVariantMap& headers = {});
	QString getErrorString(QNetworkReply* reply);
	QFuture<NetworkResponse> post(const QString& url,
		const QVariantMap& data = {},
		const QVariantMap& headers = {});
	QFuture<NetworkResponse> post(const QString& url,
		const QByteArray& data,
		const QVariantMap& headers = {});

	void setProxy(const NetworkProxy& proxy);
	void setTimeout(int milliseconds);
	void setRetryCount(int count);

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
	struct RequestContext
	{
		QString id;
		QNetworkRequest request;
		QByteArray data;
		int retryCount = 0;
		int maxRetries = 3;
		QFutureInterface<NetworkResponse> futureInterface;
		QTimer* timeoutTimer = nullptr;
		bool finished = false;
	};

	void retryRequest(std::shared_ptr<RequestContext> context);

	QString generateRequestId() const;

	bool checkPartialDownloadSupport(QNetworkReply* reply);

	QNetworkAccessManager* m_networkManager;
	QSharedPointer<ConfigManager> m_configManager;
	QNetworkCookieJar* m_cookieJar;

	QMutex m_requestsMutex;
	QHash<QString, QNetworkReply*> m_activeRequests;

	NetworkProxy m_proxy;
	int m_timeoutMs = 30000;
	int m_defaultRetryCount = 3;
	QString m_userAgent;

	static const int MAX_CONCURRENT_REQUESTS = 10;
};