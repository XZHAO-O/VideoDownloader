#pragma once

#include <atomic>
#include <QNetworkReply>

class DownloadContext
{
public:

	QNetworkAccessManager* accessManager;
	QList<QNetworkReply*> replys;
	QString fileName;
	QString url;
	std::atomic<unsigned int> downloadedPart;
	unsigned int totalPart;
	std::atomic<qint64> downloadedSize;
	qint64 fileSize;
	bool pieced;
	bool active;

	DownloadContext(QString fileName = "", QString url = "", unsigned int totalPart = 3, qint64 downloadedSize = 0, qint64 fileSize = 0)
		: accessManager(nullptr)
		, replys(QList<QNetworkReply*>())
		, fileName(fileName)
		, url(url)
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

		accessManager = new QNetworkAccessManager();
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
				delete reply;
				reply = nullptr;
			}
		}
		replys.clear();
	}

	void addNetworkReply(QNetworkReply* reply)
	{
		replys.append(reply);
	}

	bool isFinished() { return downloadedPart == totalPart; }
};
