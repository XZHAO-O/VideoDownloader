#pragma once

#include <atomic>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include "NetworkManager.h"
#include "StringUtil.h"

enum class DownloadStatus
{
	Queued = 0,
	Downloading,
	Paused,
	Completed,
	Failed
};

class DownloadContext : public QObject
{
	Q_OBJECT

public:

	QSharedPointer<NetworkManager> networkManager;
	QNetworkAccessManager* accessManager;
	QHash<int, QNetworkReply*> replys;
	QHash<int, QFile*> files;
	QString fileName;
	QUrl url;
	std::atomic<int> downloadedPart;
	int totalPart;
	qint64 progressedSize;
	QList<qint64> downloadedSize;
	std::atomic<qint64> downloadedTotalSize;
	qint64 fileSize;
	std::atomic<DownloadStatus> downloadStatus;
	bool partialDownloadSupport;
	bool active;

	DownloadContext(QString fileName = "", QUrl url = QUrl(""), int totalPart = 3)
		: QObject(nullptr)
		, accessManager(nullptr)
		, replys(QHash<int, QNetworkReply*>())
		, files(QHash<int, QFile*>())
		, fileName(fileName)
		, url(url)
		, progressedSize(0)
		, downloadedPart(0)
		, totalPart(totalPart)
		, downloadedSize(QList<qint64>(totalPart, 0))
		, downloadedTotalSize(0)
		, fileSize(0)
		, downloadStatus(DownloadStatus::Queued)
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
			qint64 rangeEnd = (i == totalPart - 1) ? fileSize - 1 : (i + 1) * partSize - 1;

			//平台header待增加
			QNetworkRequest request = networkManager->setRequest(url);
			request.setRawHeader("Range", QString("bytes=%1-%2").arg(rangeStart).arg(rangeEnd).toUtf8());
			request.setRawHeader("Referer", "https://www.bilibili.com");
			request.setRawHeader("Origin", "https://www.bilibili.com");
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
		for (auto file : files)
		{
			if (file)
			{
				if (file->isOpen())
				{
					file->close();
					file->remove();
				}
				file->deleteLater();
			}
		}
	}

private:

	void mergeFiles()
	{
		QFile* file = files[0];
		if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
		{
			qDebug() << "无法打开文件:" << fileName;
			return;
		}
		for (int i = 1; i < totalPart; i++)
		{
			QFile* partFile = files[i];
			if (!partFile->open(QIODevice::ReadOnly))
			{
				qDebug() << "无法打开文件:" << fileName + QString(".part%1").arg(i);
				return;
			}
			QByteArray data = partFile->readAll();
			qint64 bytesWritten = file->write(data);

			if (bytesWritten != data.size())
			{
				qDebug() << "写入数据不完整，分片:" << i;
				return;
			}
			partFile->close();
			partFile->remove();
			delete partFile; // 清理内存
			files[i] = nullptr;
			files.remove(i);
		}
		file->close();

		// 获取原文件名（不含后缀）
		QFileInfo fileInfo(fileName);
		QString baseName = fileInfo.completeBaseName(); // 获取不含后缀的文件名
		QString dirPath = fileInfo.absolutePath();

		// 构造新的.mp4文件路径
		QString newFilePath = dirPath + "/" + baseName + ".mp4";

		// 重命名文件 逻辑待修改
		if (QFile::exists(newFilePath))
		{
			QString timestamp = QDateTime::currentDateTime().toString("_yyyyMMdd_hhmmss");
			newFilePath = dirPath + "/" + baseName + timestamp + ".mp4";
		}

		if (!file->rename(newFilePath))
		{
			qDebug() << "重命名失败";
		}

		delete file;
		files[0] = nullptr;
		files.remove(0);
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
			delete accessManager;
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
				delete file;
				file = nullptr;
			}
		}
		files.clear();
	}

	void setTotalPart()
	{
		if (fileSize > 0)
		{
			if (fileSize < 50 * StringUtil::MB)
				totalPart = 5;
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
		QObject::connect(reply, &QNetworkReply::readyRead, this, [this, partNumber]() {
			onReadyRead(partNumber);
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
	void onReadyRead(int partNumber)
	{
		QNetworkReply* reply = replys[partNumber];
		QFile* file = files[partNumber];

		if (!file->isOpen())
		{
			if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
			{
				qDebug() << "无法打开文件:" << fileName + QString(".part%1").arg(partNumber);
				return;
			}
		}

		QByteArray data = reply->readAll();
		qint64 bytesWritten = file->write(data);

		if (bytesWritten != data.size())
		{
			qDebug() << "写入数据不完整，分片:" << partNumber << "期望:" << data.size() << "实际:" << bytesWritten;
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

		if (reply->error() == QNetworkReply::NoError)
		{
			// 确保所有数据都已写入
			if (file->isOpen())
			{
				file->write(reply->readAll());
				file->close();
			}
			downloadedPart++;

			qDebug() << "Part" << partNumber << "download completed:" << fileName;
		}
		else
		{
			onErrorOccurred(partNumber);
		}

		QObject::disconnect(reply, nullptr, this, nullptr);
		delete reply;
		replys[partNumber] = nullptr;
		replys.remove(partNumber);

		if (downloadedPart == totalPart)
		{
			// 所有分片下载完成，合并文件
			mergeFiles();
			clearNetworkResources();
			downloadStatus = DownloadStatus::Completed;
		}
	}

	void onErrorOccurred(int partNumber)
	{
		QNetworkReply* reply = replys[partNumber];
		QFile* file = files[partNumber];
		networkManager->getErrorString(reply);
		qDebug() << "Part" << partNumber << "download error:" << reply->error() << "for file" << fileName;

		if (file->isOpen())
		{
			file->close();
			file->remove();
		}
	}

signals:
	void downloadFinished();
	void downloadFailed();
};