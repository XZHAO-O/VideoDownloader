#pragma once

#include <QFuture>

struct NetworkResponse
{
	bool success;
	QByteArray data;
	QString errorString;
};

struct NetworkProxy {
	bool enabled = false;
	QString type; // "http", "socks5"
	QString host;
	int port = 0;
	QString username;
	QString password;
};

class INetworkManager {
public:
	virtual ~INetworkManager() = default;

	virtual NetworkResponse get(const QString& url,
		const QVariantMap& headers = {}) = 0;
	virtual NetworkResponse getWithLoop(const QString& url,
		const QVariantMap& headers = {}) = 0;
	virtual QFuture<NetworkResponse> post(const QString& url,
		const QVariantMap& data = {},
		const QVariantMap& headers = {}) = 0;
	virtual QFuture<NetworkResponse> post(const QString& url,
		const QByteArray& data,
		const QVariantMap& headers = {}) = 0;

	virtual void setProxy(const NetworkProxy& proxy) = 0;
	virtual void setTimeout(int milliseconds) = 0;
	virtual void setRetryCount(int count) = 0;
};