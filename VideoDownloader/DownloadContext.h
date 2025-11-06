#pragma once

#include <atomic>
#include <QNetworkReply>
#include <QFile>
#include <QObject>

#include "NetworkManager.h"
#include "StringUtil.h"

class DownloadContext : public QObject
{
	Q_OBJECT

public:

	enum class DownloadPeriod
	{
		Prepare = 0,
		Video,
		Audio,
		Merge,
	};

	QSharedPointer<NetworkManager> networkManager;
	QNetworkAccessManager* accessManager;
	QHash<int, QNetworkReply*> replys;
	QHash<int, QFile*> files;
	QString fileName;
	QString url;
	std::atomic<unsigned int> downloadedPart;
	unsigned int totalPart;
	qint64 progressedSize;
	QList<qint64> downloadedSize;
	std::atomic<qint64> downloadedTotalSize;
	qint64 fileSize;
	DownloadPeriod downloadPeriod;
	bool partialDownloadSupport;
	bool active;

	DownloadContext(QString fileName = "", QString url = "", unsigned int totalPart = 3, QList<qint64> downloadedSize = QList<qint64>(), qint64 downloadedTotalSize = 0, qint64 fileSize = 0)
		: QObject(nullptr)
		, accessManager(nullptr)
		, replys(QHash<int, QNetworkReply*>())
		, files(QHash<int, QFile*>())
		, fileName(fileName)
		, url(url)
		, progressedSize(0)
		, downloadedPart(0)
		, totalPart(totalPart)
		, downloadedSize(downloadedSize)
		, downloadedTotalSize(downloadedTotalSize)
		, fileSize(fileSize)
		, downloadPeriod(DownloadPeriod::Prepare)
		, partialDownloadSupport(false)
		, active(false)
	{

	}

	~DownloadContext()
	{
		clearNetworkResources();
	}

	void startDownload(QSharedPointer<NetworkManager> networkManager)
	{
		this->networkManager = networkManager;
		active = true;
		initNetworkResources();
		setTotalPart();
		qint64 partSize = fileSize / totalPart;
		for (int i = 0; i < totalPart; i++)
		{
			addFile(new QFile(fileName + QString(".part%1").arg(i)), i);

			qint64 rangeStart = i * partSize;
			qint64 rangeEnd = (i == totalPart - 1) ? fileSize : (i + 1) * partSize;

			QNetworkRequest request = networkManager->setRequest(url);
			request.setRawHeader("Range", QString("bytes=%1-%2").arg(rangeStart).arg(rangeEnd).toUtf8());
			QNetworkReply* reply = accessManager->get(request);
			addNetworkReply(reply, i);
		}
	}

	void stopDownload()
	{
		active = false;
		for (auto reply : replys)
		{
			if (reply)
			{
				reply->disconnect();
				if (reply->isRunning())
				{
					reply->abort();
				}
				reply->deleteLater();
			}
		}
	}

private:

	void mergeFiles()
	{
		QFile* file = new QFile(fileName);
		if (file->open(QIODevice::WriteOnly))
		{
			for (int i = 0; i < totalPart; i++)
			{
				QFile* partFile = files[i];
				if (partFile->open(QIODevice::ReadOnly))
				{
					file->write(partFile->readAll());
					partFile->close();
				}
				partFile->remove();
			}
			file->close();
		}
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

	void setTotalPart()
	{
		if (fileSize > 0)
		{
			if (fileSize < 50 * StringUtil::MB)
				totalPart = 1;
			else
			{
				if (fileSize < 100 * StringUtil::MB)
					totalPart = 2;
				else
				{
					if (fileSize < 500 * StringUtil::MB)
						totalPart = 3;
					else
					{
						if (fileSize < 1 * StringUtil::GB)
							totalPart = 4;
						else
							totalPart = 5;
					}
				}
			}
		}
		downloadedSize.resize(totalPart);
		for (int i = 0; i < totalPart; i++)
		{
			downloadedSize[i] = 0;
		}
	}

	void addNetworkReply(QNetworkReply* reply, int partNumber = 0)
	{
		replys.insert(partNumber, reply);
		setupReplyConnections(reply, partNumber);
	}

	void addFile(QFile* file, int partNumber = 0)
	{
		files.insert(partNumber, file);
	}

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
				onErrorOccurred(partNumber);
			});
	}

	// 设置单文件下载的信号连接
	void setupSingleFileConnections(QNetworkReply* reply, QFile* file, int partNumber = 1)
	{
		files.insert(partNumber, file);
		replys.insert(partNumber, reply);
		setupReplyConnections(reply, partNumber);
	}

	// 设置分片下载的信号连接
	void setupPartialFileConnections(QNetworkReply* reply, QFile* file, int partNumber = 1)
	{
		files.insert(partNumber, file);
		replys.insert(partNumber, reply);
		setupReplyConnections(reply, partNumber);
	}

private slots:
	void onReadyRead(QNetworkReply* reply, int partNumber)
	{
		QFile* file = files[partNumber];
		if (file && file->isOpen() && reply)
		{
			file->write(reply->readAll());
		}
	}

	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal, int partNumber)
	{
		// 更新下载大小统计
		int downloaded = bytesReceived - downloadedSize[partNumber];
		downloadedTotalSize += downloaded;
		downloadedSize[partNumber] = bytesReceived;
	}

	void onFinished(int partNumber)
	{
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

		if (downloadedPart == totalPart)
		{
			// 所有分片下载完成，合并文件
			mergeFiles();
			emit downloadFinished();
		}
	}

	void onErrorOccurred(int partNumber)
	{
		QNetworkReply* reply = replys[partNumber];
		QFile* file = files[partNumber];
		networkManager->getErrorString(reply);
		qDebug() << "Part" << partNumber << "download error:" << reply->error() << "for file" << fileName;

		if (file && file->isOpen()) {
			file->close();
		}
		if (file) {
			file->remove();
		}
	}

signals:
	void downloadFinished();
};