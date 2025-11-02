#include "NetworkManager.h"

#include <QJsonDocument>
#include <QRandomGenerator>
#include <QEventLoop>
#include <QTimer>
#include <QNetworkCookieJar>
#include <QNetworkProxy>
#include <QAuthenticator>

#include "ConfigManager.h"
#include "LogSystem.h"
#include "DownloadContext.h"

NetworkManager::NetworkManager(QSharedPointer<ConfigManager> configManager, QObject* parent)
	: QObject(parent)
	, m_configManager(configManager)
	, m_networkManager(new QNetworkAccessManager(this))
	, m_cookieJar(new QNetworkCookieJar(this))
{
	m_networkManager->setCookieJar(m_cookieJar);

	// 从配置加载网络设置
	m_timeoutMs = m_configManager->getValue("network/timeout", 10000).toInt();
	m_defaultRetryCount = m_configManager->getValue("network/retryCount", 3).toInt();
	m_userAgent = m_configManager->getValue("network/userAgent",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36").toString();

	// 加载代理设置
	QVariantMap proxyConfig = m_configManager->getValue("network/proxy", QVariantMap()).toMap();
	if (proxyConfig.value("enabled", false).toBool()) {
		NetworkProxy proxy;
		proxy.enabled = true;
		proxy.type = proxyConfig.value("type", "http").toString();
		proxy.host = proxyConfig.value("host").toString();
		proxy.port = proxyConfig.value("port", 0).toInt();
		proxy.username = proxyConfig.value("username").toString();
		proxy.password = proxyConfig.value("password").toString();
		setProxy(proxy);
	}

	// 连接信号
	connect(m_networkManager, &QNetworkAccessManager::authenticationRequired,
		this, &NetworkManager::onAuthenticationRequired);
	connect(m_networkManager, &QNetworkAccessManager::proxyAuthenticationRequired,
		this, &NetworkManager::onProxyAuthenticationRequired);

	LOG_INFO("Network", "Network manager initialized");
}

NetworkManager::~NetworkManager()
{
	// 取消所有活跃请求
	QMutexLocker locker(&m_requestsMutex);
	for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
		if (it.value()->timeoutTimer) {
			it.value()->timeoutTimer->stop();
			delete it.value()->timeoutTimer;
		}
		if (!it.value()->finished) {
			NetworkResponse response;
			response.success = false;
			response.errorString = "Request cancelled";
			completeRequest(it.value(), response);
		}
	}
	m_activeRequests.clear();
}

QNetworkRequest NetworkManager::setRequest(const QString& url, const QVariantMap& headers)
{
	QNetworkRequest request = QNetworkRequest(url);
	// 设置请求头
	request.setRawHeader("User-Agent", m_userAgent.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// 设置自定义头
	for (auto it = headers.cbegin(); it != headers.cend(); ++it)
		request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

	//设置超时时间
	request.setTransferTimeout(m_timeoutMs);

	return request;
}

NetworkResponse NetworkManager::get(const QString& url, const QVariantMap& headers)
{
	return NetworkResponse();
}

NetworkResponse NetworkManager::getWithLoop(const QString& url, const QVariantMap& headers)
{
	NetworkResponse networkResponse;

	QNetworkRequest request = setRequest(url, headers);

	LOG_DEBUG("Network", QString("GET request started: %1").arg(url));

	QNetworkAccessManager* networkManager = new QNetworkAccessManager();
	QNetworkReply* reply = networkManager->get(request);

	// 创建事件循环等待请求完成
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	// 检查错误
	if (reply->error() != QNetworkReply::NoError)
	{
		handleNetworkError(networkResponse, reply->error());
		networkManager->deleteLater();
		reply->deleteLater();
		return networkResponse;
	}

	// 读取响应
	networkResponse.success = true;
	networkResponse.data = reply->readAll();

	networkManager->deleteLater();
	reply->deleteLater();

	return networkResponse;
}

void NetworkManager::handleNetworkError(NetworkResponse& networkResponse, QNetworkReply::NetworkError errorCode)
{
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	if (!reply) return;

	networkResponse.success = false;

	// 根据错误类型进行不同处理
	switch (errorCode)
	{
	case QNetworkReply::ConnectionRefusedError:
		networkResponse.errorString = "服务器拒绝连接";
		break;
	case QNetworkReply::RemoteHostClosedError:
		networkResponse.errorString = "服务器关闭连接";
		break;
	case QNetworkReply::HostNotFoundError:
		networkResponse.errorString = "无法找到指定的服务器";
		break;
	case QNetworkReply::TimeoutError:
		networkResponse.errorString = "网络请求超时，请检查网络连接或稍后重试";
		break;
	case QNetworkReply::OperationCanceledError:
		networkResponse.errorString = "网络请求已被取消";
		break;
	case QNetworkReply::SslHandshakeFailedError:
		networkResponse.errorString = "SSL连接建立失败";
		break;
	case QNetworkReply::TemporaryNetworkFailureError:
		networkResponse.errorString = "检测到临时网络故障";
		break;
	case QNetworkReply::NetworkSessionFailedError:
		networkResponse.errorString = "网络会话初始化失败";
		break;
	case QNetworkReply::BackgroundRequestNotAllowedError:
		networkResponse.errorString = "当前环境不允许后台网络请求";
		break;
	case QNetworkReply::TooManyRedirectsError:
		networkResponse.errorString = "请求经历了太多重定向";
		break;
	case QNetworkReply::InsecureRedirectError:
		networkResponse.errorString = "检测到不安全的HTTP重定向";
		break;
	case QNetworkReply::ProxyConnectionRefusedError:
		networkResponse.errorString = "代理服务器拒绝连接";
		break;
	case QNetworkReply::ProxyConnectionClosedError:
		networkResponse.errorString = "代理服务器在操作期间关闭了连接";
		break;
	case QNetworkReply::ProxyNotFoundError:
		networkResponse.errorString = "无法找到指定的代理服务器";
		break;
	case QNetworkReply::ProxyTimeoutError:
		networkResponse.errorString = "与代理服务器的连接超时";
		break;
	case QNetworkReply::ProxyAuthenticationRequiredError:
		networkResponse.errorString = "代理服务器需要身份验证";
		break;
	case QNetworkReply::ContentAccessDenied:
		networkResponse.errorString = "访问请求的资源被拒绝";
		break;
	case QNetworkReply::ContentOperationNotPermittedError:
		networkResponse.errorString = "请求的操作在资源上不被允许";
		break;
	case QNetworkReply::ContentNotFoundError:
		networkResponse.errorString = "请求的资源在服务器上未找到";
		break;
	case QNetworkReply::AuthenticationRequiredError:
		networkResponse.errorString = "服务器需要身份验证";
		break;
	case QNetworkReply::ContentReSendError:
		networkResponse.errorString = "无法重新发送请求";
		break;
	case QNetworkReply::ContentConflictError:
		networkResponse.errorString = "请求与资源的当前状态冲突";
		break;
	case QNetworkReply::ContentGoneError:
		networkResponse.errorString = "请求的资源不存在";
		break;
	case QNetworkReply::InternalServerError:
		networkResponse.errorString = "服务器内部错误";
		break;
	case QNetworkReply::OperationNotImplementedError:
		networkResponse.errorString = "服务器不支持请求的操作";
		break;
	case QNetworkReply::ServiceUnavailableError:
		networkResponse.errorString = "服务器暂时不可用，请稍后重试";
		break;
	case QNetworkReply::ProtocolUnknownError:
		networkResponse.errorString = "网络协议未知";
		break;
	case QNetworkReply::ProtocolInvalidOperationError:
		networkResponse.errorString = "请求的操作对当前协议无效";
		break;
	case QNetworkReply::UnknownNetworkError:
		networkResponse.errorString = "发生未知的网络错误";
		break;
	case QNetworkReply::UnknownProxyError:
		networkResponse.errorString = "代理服务器报告未知错误";
		break;
	case QNetworkReply::UnknownContentError:
		networkResponse.errorString = "与内容相关的未知错误";
		break;
	case QNetworkReply::ProtocolFailure:
		networkResponse.errorString = "协议处理失败";
		break;
	case QNetworkReply::UnknownServerError:
		networkResponse.errorString = "服务器报告未知错误";
		break;
	default:
		networkResponse.errorString = QString("发生未处理错误 [%1]: %2")
			.arg(errorCode)
			.arg(reply->errorString());
		break;
	}

	// 记录错误日志
	LOG_ERROR("NetworkManager", QString("错误代码: %1：%2 ")
		.arg(errorCode)
		.arg(networkResponse.errorString));
}

QFuture<NetworkResponse> NetworkManager::post(const QString& url, const QVariantMap& data, const QVariantMap& headers)
{
	QJsonDocument doc(QJsonObject::fromVariantMap(data));
	return post(url, doc.toJson(), headers);
}

QFuture<NetworkResponse> NetworkManager::post(const QString& url, const QByteArray& data, const QVariantMap& headers)
{
	QFutureInterface<NetworkResponse> futureInterface;
	futureInterface.reportStarted();
	QFuture<NetworkResponse> future = futureInterface.future();

	if (m_activeRequests.size() >= MAX_CONCURRENT_REQUESTS) {
		NetworkResponse response;
		response.success = false;
		response.errorString = "Too many concurrent requests";
		futureInterface.reportResult(response);
		futureInterface.reportFinished();
		return future;
	}

	auto context = std::make_shared<RequestContext>();
	context->id = generateRequestId();
	context->request = QNetworkRequest(QUrl(url));
	context->data = data;
	context->maxRetries = m_defaultRetryCount;
	context->futureInterface = futureInterface;
	context->finished = false;

	// 设置请求头
	context->request.setRawHeader("User-Agent", m_userAgent.toUtf8());
	context->request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	// 设置自定义头
	for (auto it = headers.begin(); it != headers.end(); ++it) {
		context->request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
	}

	// 设置超时定时器
	context->timeoutTimer = new QTimer(this);
	context->timeoutTimer->setSingleShot(true);
	connect(context->timeoutTimer, &QTimer::timeout, this, [this, context]() {
		NetworkResponse response;
		response.success = false;
		response.errorString = "Request timeout";
		completeRequest(context, response);
		});
	context->timeoutTimer->start(m_timeoutMs);

	{
		QMutexLocker locker(&m_requestsMutex);
		m_activeRequests[context->id] = context;
	}

	LOG_DEBUG("Network", QString("POST request started: %1, data size: %2").arg(url).arg(data.size()));

	QNetworkReply* reply = m_networkManager->post(context->request, data);
	handleReply(reply, context);

	return future;
}

void NetworkManager::setProxy(const NetworkProxy& proxy)
{
	m_proxy = proxy;

	if (proxy.enabled) {
		QNetworkProxy networkProxy;

		if (proxy.type == "http") {
			networkProxy.setType(QNetworkProxy::HttpProxy);
		}
		else if (proxy.type == "socks5") {
			networkProxy.setType(QNetworkProxy::Socks5Proxy);
		}
		else {
			networkProxy.setType(QNetworkProxy::DefaultProxy);
		}

		networkProxy.setHostName(proxy.host);
		networkProxy.setPort(proxy.port);

		if (!proxy.username.isEmpty()) {
			networkProxy.setUser(proxy.username);
			networkProxy.setPassword(proxy.password);
		}

		m_networkManager->setProxy(networkProxy);
		LOG_INFO("Network", QString("Proxy set: %1:%2").arg(proxy.host).arg(proxy.port));
	}
	else {
		m_networkManager->setProxy(QNetworkProxy::NoProxy);
		LOG_INFO("Network", "Proxy disabled");
	}

	// 保存到配置
	QVariantMap proxyConfig;
	proxyConfig["enabled"] = proxy.enabled;
	proxyConfig["type"] = proxy.type;
	proxyConfig["host"] = proxy.host;
	proxyConfig["port"] = proxy.port;
	proxyConfig["username"] = proxy.username;
	proxyConfig["password"] = proxy.password;

	m_configManager->setValue("network/proxy", proxyConfig);
}

void NetworkManager::setTimeout(int milliseconds)
{
	m_timeoutMs = milliseconds;
	m_configManager->setValue("network/timeout", milliseconds);
	LOG_DEBUG("Network", QString("Timeout set to: %1 ms").arg(milliseconds));
}

void NetworkManager::setRetryCount(int count)
{
	m_defaultRetryCount = count;
	m_configManager->setValue("network/retryCount", count);
	LOG_DEBUG("Network", QString("Retry count set to: %1").arg(count));
}

void NetworkManager::setCookies(const QString& domain, const QList<QNetworkCookie>& cookies)
{
	// 使用 QNetworkCookieJar 的现有方法设置 cookies
	for (const QNetworkCookie& cookie : cookies) {
		m_cookieJar->insertCookie(cookie);
	}
	LOG_DEBUG("Network", QString("Cookies set for domain: %1, count: %2").arg(domain).arg(cookies.size()));
}

QList<QNetworkCookie> NetworkManager::getCookies(const QString& domain) const
{
	return m_cookieJar->cookiesForUrl(QUrl(domain));
}

void NetworkManager::clearCookies()
{
	// 创建一个空的 cookie jar 来替换当前的
	QNetworkCookieJar* newCookieJar = new QNetworkCookieJar(this);
	m_networkManager->setCookieJar(newCookieJar);
	delete m_cookieJar;
	m_cookieJar = newCookieJar;
	LOG_INFO("Network", "All cookies cleared");
}

void NetworkManager::setUserAgent(const QString& userAgent)
{
	m_userAgent = userAgent;
	m_configManager->setValue("network/userAgent", userAgent);
	LOG_DEBUG("Network", QString("User agent set: %1").arg(userAgent));
}

QString NetworkManager::userAgent() const
{
	return m_userAgent;
}

void NetworkManager::onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
	Q_UNUSED(reply)
		Q_UNUSED(authenticator)
		LOG_WARN("Network", "Authentication required");
}

void NetworkManager::onProxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator)
{
	if (!m_proxy.username.isEmpty()) {
		authenticator->setUser(m_proxy.username);
		authenticator->setPassword(m_proxy.password);
		LOG_DEBUG("Network", "Proxy authentication provided");
	}
	else {
		LOG_WARN("Network", "Proxy authentication required but no credentials provided");
	}
}

void NetworkManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
	QStringList errorStrings;
	for (const QSslError& error : errors) {
		errorStrings << error.errorString();
	}

	LOG_ERROR("Network", QString("SSL errors: %1").arg(errorStrings.join("; ")));
	reply->ignoreSslErrors(); // 忽略SSL错误（在生产环境中应该更谨慎）
}

void NetworkManager::handleReply(QNetworkReply* reply, std::shared_ptr<RequestContext> context)
{
	//// 连接完成信号
	//connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
	//	NetworkResponse response;

	//	if (reply->error() == QNetworkReply::NoError) {
	//		response.success = true;
	//		response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	//		response.data = reply->readAll();

	//		// 获取响应头
	//		QList<QByteArray> headerList = reply->rawHeaderList();
	//		for (const QByteArray& header : headerList) {
	//			response.headers[QString::fromUtf8(header)] = QString::fromUtf8(reply->rawHeader(header));
	//		}

	//		LOG_DEBUG("Network", QString("Request succeeded: %1, status: %2, size: %3")
	//			.arg(context->request.url().toString())
	//			.arg(response.statusCode)
	//			.arg(response.data.size()));

	//		completeRequest(context, response);
	//	}
	//	else {
	//		response.success = false;
	//		response.statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	//		response.errorString = reply->errorString();

	//		LOG_WARN("Network", QString("Request failed: %1, error: %2, status: %3")
	//			.arg(context->request.url().toString())
	//			.arg(response.errorString)
	//			.arg(response.statusCode));

	//		// 检查是否应该重试
	//		if (context->retryCount < context->maxRetries &&
	//			(reply->error() == QNetworkReply::TimeoutError ||
	//				reply->error() == QNetworkReply::ConnectionRefusedError ||
	//				reply->error() == QNetworkReply::RemoteHostClosedError)) {

	//			context->retryCount++;
	//			LOG_INFO("Network", QString("Retrying request (%1/%2): %3")
	//				.arg(context->retryCount)
	//				.arg(context->maxRetries)
	//				.arg(context->request.url().toString()));

	//			// 取消超时定时器
	//			if (context->timeoutTimer) {
	//				context->timeoutTimer->stop();
	//			}

	//			// 延迟后重试
	//			QTimer::singleShot(1000 * context->retryCount, this, [this, context]() {
	//				retryRequest(context);
	//				});
	//		}
	//		else {
	//			completeRequest(context, response);
	//		}
	//	}

	//	reply->deleteLater();
	//	});
}

void NetworkManager::retryRequest(std::shared_ptr<RequestContext> context)
{
	// 重新启动超时定时器
	if (context->timeoutTimer) {
		context->timeoutTimer->start(m_timeoutMs);
	}

	QNetworkReply* reply;
	if (context->data.isEmpty()) {
		reply = m_networkManager->get(context->request);
	}
	else {
		reply = m_networkManager->post(context->request, context->data);
	}

	handleReply(reply, context);
}

void NetworkManager::completeRequest(std::shared_ptr<RequestContext> context, const NetworkResponse& response)
{
	if (context->finished) {
		return; // 避免重复完成
	}
	context->finished = true;

	// 停止并清理超时定时器
	if (context->timeoutTimer) {
		context->timeoutTimer->stop();
		context->timeoutTimer->deleteLater();
		context->timeoutTimer = nullptr;
	}

	// 从活跃请求中移除
	{
		QMutexLocker locker(&m_requestsMutex);
		m_activeRequests.remove(context->id);
	}

	// 报告结果
	context->futureInterface.reportResult(response);
	context->futureInterface.reportFinished();
}

QString NetworkManager::generateRequestId() const
{
	return QString::number(QDateTime::currentMSecsSinceEpoch()) +
		QString::number(QRandomGenerator::global()->generate64());
}

QNetworkReply* NetworkManager::download(DownloadContext& context)
{
	LOG_INFO("Network", QString("Starting download: %1 -> %2").arg(context.url).arg(context.fileName));

	QNetworkRequest request = setRequest(context.url);
	if (context.pieced)
	{
		//request.setRawHeader("Range", QString("bytes=%1-%2").arg(context.rangeStart).arg(context.rangeEnd).toUtf8());
	}
	QNetworkReply* reply = context.accessManager->get(request);

	return reply;
}

bool NetworkManager::checkPartialDownloadSupport(const QString& url)
{
	// 发送HEAD请求检查是否支持Range头
	QNetworkRequest request(url);
	request.setRawHeader("User-Agent", m_userAgent.toUtf8());

	QNetworkAccessManager tempManager;
	QNetworkReply* reply = tempManager.head(request);

	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	bool supportsPartial = false;
	if (reply->error() == QNetworkReply::NoError) {
		QByteArray acceptRanges = reply->rawHeader("Accept-Ranges");
		QString contentLength = reply->rawHeader("Content-Length");

		supportsPartial = (acceptRanges == "bytes" && !contentLength.isEmpty());
		LOG_DEBUG("Network", QString("Partial download support: %1, Content-Length: %2")
			.arg(supportsPartial ? "yes" : "no").arg(contentLength));
	}
	else {
		LOG_WARN("Network", QString("Failed to check partial download support: %1").arg(reply->errorString()));
	}

	reply->deleteLater();
	return supportsPartial;
}

void NetworkManager::downloadSingleFile(DownloadContext& context)
{
	QNetworkRequest request(context.url);
	request.setRawHeader("User-Agent", m_userAgent.toUtf8());

	QNetworkReply* reply = context.accessManager->get(request);

	// 创建文件对象
	QFile* file = new QFile(context.fileName, &context);
	if (!file->open(QIODevice::WriteOnly)) {
		qDebug() << "Failed to open file for writing:" << context.fileName << file->errorString();
		delete file;
		reply->deleteLater();
		emit context.downloadFinished(false, "Failed to create file");
		return;
	}

	// 设置信号连接（全部在DownloadContext内部处理）
	context.setupSingleFileConnections(reply, file);
}

void NetworkManager::downloadWithRange(DownloadContext& context, qint64 rangeStart, qint64 rangeEnd, int partNumber)
{
	QNetworkRequest request(context.url);
	request.setRawHeader("User-Agent", m_userAgent.toUtf8());

	// 设置范围请求
	QString rangeHeader;
	if (rangeEnd >= 0) {
		rangeHeader = QString("bytes=%1-%2").arg(rangeStart).arg(rangeEnd);
	}
	else {
		rangeHeader = QString("bytes=%1-").arg(rangeStart);
	}
	request.setRawHeader("Range", rangeHeader.toUtf8());

	qDebug() << "Downloading part" << partNumber << ":" << rangeHeader << "->" << context.fileName;

	QNetworkReply* reply = context.accessManager->get(request);

	// 创建分片文件名
	QString partFilename = "1";
	QFile* file = new QFile(partFilename, &context);

	if (!file->open(QIODevice::WriteOnly)) {
		qDebug() << "Failed to open part file for writing:" << partFilename << file->errorString();
		delete file;
		reply->deleteLater();
		emit context.downloadPartFinished(partNumber, context.totalPart, false, "Failed to create part file");
		return;
	}

	// 设置信号连接（全部在DownloadContext内部处理）
	context.setupPartialFileConnections(reply, file, partNumber);
}

void NetworkManager::downloadPartialFile(DownloadContext& context, int partNumber)
{
	// 计算分片大小和范围
	qint64 partSize = context.fileSize / context.totalPart;
	qint64 rangeStart = partNumber * partSize;
	qint64 rangeEnd = (partNumber == context.totalPart - 1) ? -1 : (partNumber + 1) * partSize - 1;

	downloadWithRange(context, rangeStart, rangeEnd, partNumber);
}

void NetworkManager::mergeDownloadedFiles(const QString& filename)
{
	//QFile finalFile(filename);
	//if (!finalFile.open(QIODevice::WriteOnly)) {
	//	LOG_ERROR("Network", QString("Failed to create final file: %1").arg(filename));
	//	emit downloadFinished(filename, false, "Failed to create final file");
	//	cleanupPartFiles(filename);
	//	return;
	//}

	//bool success = true;
	//QString errorString;

	//// 按顺序合并所有分片文件
	//for (int i = 0; i < 3; ++i) {
	//	QFile partFile(filename + QString("%1").arg(i));
	//	if (!partFile.open(QIODevice::ReadOnly)) {
	//		LOG_ERROR("Network", QString("Failed to open part file: %1").arg(i));
	//		success = false;
	//		errorString = QString("Failed to open part file: %1").arg(i);
	//		break;
	//	}

	//	// 读取分片内容并写入最终文件
	//	QByteArray data = partFile.readAll();
	//	if (finalFile.write(data) != data.size()) {
	//		LOG_ERROR("Network", QString("Failed to write part %1 data").arg(i));
	//		success = false;
	//		errorString = QString("Failed to write part %1 data").arg(i);
	//		partFile.close();
	//		break;
	//	}

	//	partFile.close();
	//}

	//finalFile.close();

	//if (success) {
	//	LOG_INFO("Network", QString("File merged successfully: %1, size: %2 bytes")
	//		.arg(filename).arg(finalFile.size()));
	//	// 清理分片文件
	//	cleanupPartFiles(filename);
	//}
	//else {
	//	// 删除不完整的最终文件
	//	finalFile.remove();
	//	cleanupPartFiles(filename);
	//}

	//emit downloadFinished(filename, success, errorString);
}