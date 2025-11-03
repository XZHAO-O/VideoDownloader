#pragma once

#include <atomic>
#include <QNetworkReply>
#include <QFile>
#include <QObject>

#include "NetworkManager.h"

class DownloadContext : public QObject
{
	Q_OBJECT

public:

	enum class DownloadPeriod
	{
		Video = 0,
		Audio = 1,
		Merge = 2
	};

	QNetworkAccessManager* accessManager;
	QList<QNetworkReply*> replys;
	QList<QFile*> files;
	QString fileName;
	QString url;
	std::atomic<unsigned int> downloadedPart;
	unsigned int totalPart;
	qint64 progressedSize;
	std::atomic<qint64> downloadedSize;
	qint64 fileSize;
	DownloadPeriod downloadPeriod;
	bool pieced;
	bool active;

	DownloadContext(QString fileName = "", QString url = "", unsigned int totalPart = 3, qint64 downloadedSize = 0, qint64 fileSize = 0)
		: QObject(nullptr)
		, accessManager(nullptr)
		, replys(QList<QNetworkReply*>())
		, files(QList<QFile*>())
		, fileName(fileName)
		, url(url)
		, progressedSize(0)
		, downloadedPart(0)
		, totalPart(totalPart)
		, downloadedSize(downloadedSize)
		, fileSize(fileSize)
		, pieced(false)
		, active(false)
	{

	}

	~DownloadContext()
	{
		clearNetworkResources();
	}

	void initNetworkResources()
	{
		clearNetworkResources();
		accessManager = new QNetworkAccessManager(this);
	}

	void clearNetworkResources()
	{
		if (accessManager)
		{
			accessManager->disconnect();
			accessManager->deleteLater();
			accessManager = nullptr;
		}

		for (auto reply : replys)
		{
			if (reply)
			{
				reply->disconnect();
				if (reply->isRunning())
					reply->abort();
				reply->deleteLater();
			}
		}
		replys.clear();

		for (auto file : files)
		{
			if (file)
			{
				if (file->isOpen())
					file->close();
				file->deleteLater();
			}
		}
		files.clear();
	}

	void addNetworkReply(QNetworkReply* reply, int partNumber = 0)
	{
		replys.append(reply);
		setupReplyConnections(reply, partNumber);
	}

	void addFile(QFile* file)
	{
		files.append(file);
	}

	bool isFinished() { return downloadedPart == totalPart; }

	// 设置单个下载任务的信号连接
	void setupReplyConnections(QNetworkReply* reply, int partNumber)
	{
		// 连接 readyRead 信号
		QObject::connect(reply, &QNetworkReply::readyRead, this, [this, reply, partNumber]() {
			onReadyRead(reply, partNumber);
			});

		// 连接 downloadProgress 信号
		QObject::connect(reply, &QNetworkReply::downloadProgress, this, [this, partNumber](qint64 bytesReceived, qint64 bytesTotal) {
			onDownloadProgress(bytesReceived, bytesTotal, partNumber);
			});

		// 连接 finished 信号
		QObject::connect(reply, &QNetworkReply::finished, this, [this, partNumber]() {
			onFinished(partNumber);
			});

		// 连接 errorOccurred 信号
		QObject::connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
			this, [this, partNumber](QNetworkReply::NetworkError error) {
				onErrorOccurred(error, partNumber);
			});
	}

	// 设置单文件下载的信号连接
	void setupSingleFileConnections(QNetworkReply* reply, QFile* file)
	{
		files.append(file);
		replys.append(reply);
		setupReplyConnections(reply, 0);
	}

	// 设置分片下载的信号连接
	void setupPartialFileConnections(QNetworkReply* reply, QFile* file, int partNumber)
	{
		files.append(file);
		replys.append(reply);
		setupReplyConnections(reply, partNumber);
	}

private slots:
	void onReadyRead(QNetworkReply* reply, int partNumber)
	{
		if (partNumber < 0 || partNumber >= files.size())
			return;

		QFile* file = files[partNumber];
		if (file && file->isOpen() && reply) {
			file->write(reply->readAll());
		}
	}

	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal, int partNumber)
	{
		Q_UNUSED(bytesTotal)
			// 更新下载大小统计
			downloadedSize += bytesReceived;

		// 转换为MB显示
		QString receivedBytes = QString::number(downloadedSize / (1024 * 1024.0), 'f', 2);
		QString totalBytes = fileSize > 0 ? QString::number(fileSize / (1024 * 1024.0), 'f', 2) : "未知";

		QString progressInfo = fileSize > 0 ? QString("%1/%2 MB").arg(receivedBytes).arg(totalBytes) : QString("%1 MB").arg(receivedBytes);
		double progress = fileSize > 0 ? (double)downloadedSize / fileSize : 0.0;

		emit downloadProgress(progressInfo, progress, partNumber);
	}

	void onFinished(int partNumber)
	{
		if (partNumber < 0 || partNumber >= replys.size() || partNumber >= files.size())
			return;

		QNetworkReply* reply = replys[partNumber];
		QFile* file = files[partNumber];

		bool success = false;
		QString errorString;

		if (reply && reply->error() == QNetworkReply::NoError) {
			// 确保所有数据都已写入
			if (file && file->isOpen()) {
				file->write(reply->readAll());
				file->close();
			}
			success = true;
			downloadedPart++;

			qDebug() << "Part" << partNumber << "download completed:" << fileName;
		}
		else {
			if (reply) {
				errorString = reply->errorString();
			}
			if (file && file->isOpen()) {
				file->close();
			}
			// 删除不完整的文件
			if (file) {
				file->remove();
			}
		}

		if (reply) {
			reply->deleteLater();
		}

		emit downloadPartFinished(partNumber, totalPart, success, errorString);

		if (isFinished()) {
			emit downloadFinished(success, errorString);
		}
	}

	void onErrorOccurred(QNetworkReply::NetworkError error, int partNumber)
	{
		Q_UNUSED(error)

			if (partNumber < 0 || partNumber >= replys.size() || partNumber >= files.size())
				return;

		QNetworkReply* reply = replys[partNumber];
		QFile* file = files[partNumber];

		qDebug() << "Part" << partNumber << "download error:" << error << "for file" << fileName;

		if (file && file->isOpen()) {
			file->close();
		}
		if (file) {
			file->remove();
		}

		emit downloadPartFinished(partNumber, totalPart, false, QString("Network error: %1").arg(error));
	}

signals:
	void downloadProgress(const QString& progressInfo, double progress, int partNumber);
	void downloadPartFinished(int partNumber, int totalParts, bool success, const QString& errorString);
	void downloadFinished(bool success, const QString& errorString);
};