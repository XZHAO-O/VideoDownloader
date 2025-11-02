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

	QNetworkRequest setRequest(const QString& url, const QVariantMap& headers = {});

	// 网络请求方法
	QNetworkReply* download(DownloadContext& context);
	NetworkResponse get(const QString& url,
		const QVariantMap& headers = {});
	NetworkResponse getWithLoop(const QString& url,
		const QVariantMap& headers = {});
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
	void handleNetworkError(NetworkResponse& networkResponse, QNetworkReply::NetworkError errorCode);

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

	void handleReply(QNetworkReply* reply, std::shared_ptr<RequestContext> context);
	void retryRequest(std::shared_ptr<RequestContext> context);
	void completeRequest(std::shared_ptr<RequestContext> context, const NetworkResponse& response);

	QString generateRequestId() const;

	bool checkPartialDownloadSupport(const QString& url);

	void downloadSingleFile(DownloadContext& context);

	void downloadPartialFile(DownloadContext& context, int partNumber);

	void downloadWithRange(DownloadContext& context, qint64 rangeStart, qint64 rangeEnd, int partNumber);

	void mergeDownloadedFiles(const QString& filename);

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