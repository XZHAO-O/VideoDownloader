#pragma once

#include <atomic>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QCryptographicHash>

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

private:
	// 分片信息结构体
	struct PartInfo
	{
		QFile* file = nullptr;     // 分片文件对象
		qint64 downloadedSize = 0; // 该分片已下载大小
		QNetworkReply* reply = nullptr; // 对应的网络回复
		QCryptographicHash* hash = nullptr; // 对应的md5校验对象
		QString md5;

		PartInfo()
		{
		}

		~PartInfo()
		{
			clear();
		}

		// 清理资源
		void clear(bool deleteFile = false)
		{
			if (reply)
			{
				reply->disconnect();
				if (reply->isRunning())
				{
					reply->abort();
				}
				reply->deleteLater();
				reply = nullptr;
			}

			if (file)
			{
				if (file->isOpen())
				{
					file->close();
				}
				if (deleteFile)
				{
					file->remove();
				}
				delete file;
				file = nullptr;
			}
			if (hash)
			{
				if (!deleteFile)
					md5 = hash->result().toHex();
				delete hash;
				hash = nullptr;
			}
		}

		// 判断是否有效
		bool isValid() const
		{
			return file != nullptr || reply != nullptr;
		}
	};

public:
	QSharedPointer<NetworkManager> networkManager;
	QNetworkAccessManager* accessManager;
	QString fileName;
	QUrl url;
	std::atomic<int> downloadedPart;
	int totalPart;
	qint64 progressedSize;
	std::atomic<qint64> downloadedTotalSize;
	qint64 fileSize;
	std::atomic<DownloadStatus> downloadStatus;
	bool partialDownloadSupport;
	bool active;
	QList<PartInfo> partInfoList;

	DownloadContext(QString fileName = "", QUrl url = QUrl(), int totalPart = 3)
		: QObject(nullptr)
		, accessManager(nullptr)
		, fileName(fileName)
		, url(url)
		, progressedSize(0)
		, downloadedPart(0)
		, totalPart(totalPart)
		, downloadedTotalSize(0)
		, fileSize(0)
		, downloadStatus(DownloadStatus::Queued)
		, partialDownloadSupport(true)
		, active(false)
		, partInfoList(totalPart)
	{
	}

	~DownloadContext()
	{
		clearResource();
	}

	void startDownload(QSharedPointer<NetworkManager> networkManager)
	{
		if (downloadStatus == DownloadStatus::Downloading || downloadStatus == DownloadStatus::Completed) return;

		downloadStatus = DownloadStatus::Downloading;
		this->networkManager = networkManager;
		active = true;

		setTotalPart();
		// 重新调整列表大小
		partInfoList.resize(totalPart);

		accessManager = new QNetworkAccessManager();
		qint64 partSize = fileSize / totalPart;
		for (int i = 0; i < totalPart; i++)
		{
			auto& partInfo = partInfoList[i];
			// 创建文件对象
			QFile* file = new QFile(fileName + QString(".part%1").arg(i));
			partInfo.file = file;

			if (partInfo.hash)
			{
				delete partInfo.hash;
			}
			partInfo.hash = new QCryptographicHash(QCryptographicHash::Md5);

			if (!checkFileInfo(partInfo))
			{
				qDebug() << "已下载文件不存在或损坏:" << file->fileName() << "重新下载";
				file->remove();
				partInfo.downloadedSize = 0;
			}

			qint64 rangeStart = i * partSize;
			qint64 rangeEnd = (i == totalPart - 1) ? fileSize - 1 : (i + 1) * partSize - 1;
			qint64 size = rangeEnd - rangeStart + 1;
			if (size == partInfo.downloadedSize)
			{
				continue;
			}
			rangeStart += partInfo.downloadedSize;

			if (!file->open(QIODevice::WriteOnly | QIODevice::Append))
			{
				qDebug() << "无法打开文件:" << file->fileName();
				downloadStatus = DownloadStatus::Failed;
				clearResource();
				return;
			}

			//平台header待增加
			QNetworkRequest request = networkManager->setRequest(url);
			request.setRawHeader("Range", QString("bytes=%1-%2").arg(rangeStart).arg(rangeEnd).toUtf8());
			request.setRawHeader("Referer", "https://www.bilibili.com");
			request.setRawHeader("Origin", "https://www.bilibili.com");
			QNetworkReply* reply = accessManager->get(request);

			// 设置回复对象并连接信号
			partInfo.reply = reply;
			setupReplyConnections(reply, i);
		}
	}

	void pauseDownload()
	{
		if (downloadStatus == DownloadStatus::Paused || downloadStatus == DownloadStatus::Completed) return;

		downloadStatus = DownloadStatus::Paused;
		active = false;

		clearResource();
	}

	void cancelDownload()
	{
		if (downloadStatus == DownloadStatus::Failed || downloadStatus == DownloadStatus::Completed) return;

		downloadStatus = DownloadStatus::Failed;
		active = false;

		clearResource(true);
	}

private:

	void mergeFiles()
	{
		QFile* mainFile = partInfoList[0].file;
		if (!mainFile->open(QIODevice::WriteOnly | QIODevice::Append))
		{
			qDebug() << "无法打开文件:" << fileName;
			return;
		}

		constexpr qint64 bufferSize = 2 * StringUtil::MB;
		QByteArray buffer(bufferSize, Qt::Uninitialized);

		for (int i = 1; i < totalPart; i++)
		{
			QFile* partFile = partInfoList[i].file;
			if (!partFile || !partFile->open(QIODevice::ReadOnly))
			{
				qDebug() << "无法打开文件:" << partInfoList[i].file->fileName();
				return;
			}

			while (!partFile->atEnd())
			{
				qint64 bytesRead = partFile->read(buffer.data(), bufferSize);
				qint64 bytesWritten = mainFile->write(buffer.constData(), bytesRead);
				if (bytesWritten != bytesRead)
				{
					qDebug() << "写入数据不完整，分片:" << i;
					return;
				}
			}
			partFile->close();
			mainFile->flush();
			partFile->remove();
			delete partFile;
			partInfoList[i].file = nullptr;
		}
		mainFile->close();

		// 获取原文件名（不含后缀）
		QFileInfo fileInfo(fileName);
		QString baseName = fileInfo.completeBaseName(); // 获取不含后缀的文件名
		QString suffix = fileInfo.suffix();//获取后缀名
		QString dirPath = fileInfo.absolutePath();

		QString newFilePath = fileName;
		// 重命名文件 逻辑待修改
		if (QFile::exists(newFilePath))
		{
			QString timestamp = QDateTime::currentDateTime().toString("_yyyyMMdd_hhmmss");
			newFilePath = dirPath + "/" + baseName + timestamp + suffix;
		}

		if (!mainFile->rename(newFilePath))
		{
			qDebug() << "重命名失败";
		}

		delete mainFile;
		partInfoList[0].file = nullptr;
	}

	void clearResource(bool deleteFile = false)
	{
		if (accessManager)
		{
			accessManager->disconnect();
			accessManager->deleteLater();
			accessManager = nullptr;
		}

		for (auto& partInfo : partInfoList)
		{
			partInfo.clear(deleteFile);
		}
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
	}

	bool checkFileInfo(PartInfo& partInfo)
	{
		if (partInfo.downloadedSize == 0)
			return true;
		// 检查文件是否存在
		QFile* file = partInfo.file;
		if (!file->exists())
		{
			return false;
		}

		// 计算整个文件哈希
		QCryptographicHash* hash = new QCryptographicHash(QCryptographicHash::Md5);
		if (file->open(QIODevice::ReadOnly))
		{
			constexpr qint64 bufferSize = StringUtil::MB;
			QByteArray buffer(bufferSize, Qt::Uninitialized);  // 一次性分配，不初始化内容

			while (!file->atEnd())
			{
				qint64 bytesRead = file->read(buffer.data(), bufferSize);
				hash->addData(QByteArrayView(buffer.constData(), bytesRead));
			}
			file->close();

			if (QString(hash->result().toHex()) == partInfo.md5)
			{
				delete partInfo.hash;
				partInfo.hash = hash;
				partInfo.downloadedSize = file->size();
				return true;
			}
		}
		delete hash;
		return false;
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
		QObject::connect(reply, &QNetworkReply::errorOccurred,
			this, [this, partNumber](QNetworkReply::NetworkError error) {
				onErrorOccurred(partNumber);
			});
	}

private slots:
	void onReadyRead(int partNumber)
	{
		PartInfo& partInfo = partInfoList[partNumber];
		QNetworkReply* reply = partInfo.reply;
		QFile* file = partInfo.file;

		if (!reply || !file)
			return;

		// 使用预分配的缓冲区，避免频繁内存分配
		constexpr qint64 bufferSize = 64 * StringUtil::KB;
		QByteArray buffer(bufferSize, Qt::Uninitialized);

		qint64 bytesRead = 0;
		while ((bytesRead = reply->read(buffer.data(), bufferSize)) > 0)
		{
			qint64 bytesWritten = file->write(buffer.constData(), bytesRead);
			if (bytesWritten != bytesRead)
			{
				qDebug() << "写入数据不完整，分片:" << partNumber;
				downloadStatus = DownloadStatus::Failed;
				clearResource();
				return;
			}
			partInfo.hash->addData(QByteArrayView(buffer.constData(), bytesRead));
		}
	}

	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal, int partNumber)
	{
		PartInfo& partInfo = partInfoList[partNumber];
		// 更新下载大小统计
		int downloaded = bytesReceived - partInfo.downloadedSize;
		downloadedTotalSize += downloaded;
		partInfo.downloadedSize = bytesReceived;
	}

	void onFinished(int partNumber)
	{
		PartInfo& partInfo = partInfoList[partNumber];
		QNetworkReply* reply = partInfo.reply;
		QFile* file = partInfo.file;

		if (!reply || !file)
			return;

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
		reply->deleteLater();
		partInfo.reply = nullptr;

		if (downloadedPart == totalPart)
		{
			// 所有分片下载完成，合并文件
			mergeFiles();
			clearResource();
			downloadStatus = DownloadStatus::Completed;
		}
	}

	void onErrorOccurred(int partNumber)
	{
		if (partNumber < 0 || partNumber >= partInfoList.size())
			return;

		PartInfo& partInfo = partInfoList[partNumber];
		QNetworkReply* reply = partInfo.reply;
		QFile* file = partInfo.file;

		if (!reply || !file)
			return;

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