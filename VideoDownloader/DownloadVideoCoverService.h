#pragma once

#include <QObject>
#include <QSharedPointer>

#include "DownloadVideoCover.h"

class DatabaseManager;
class QueryWrapper;
class DownloadVideoCoverDAO;
class DownloadTaskInfo;

typedef DownloadTaskInfo DownloadVideoCoverReq;

class DownloadVideoCoverService : public QObject
{
	Q_OBJECT

public:
	explicit DownloadVideoCoverService(QSharedPointer<DatabaseManager> dbManager,
		QObject* parent = nullptr);
	~DownloadVideoCoverService();

	DownloadVideoCover generateFromReq(const DownloadVideoCoverReq& req);

	bool insert(const DownloadVideoCover& downloadVideoCover);
	bool insert(const QList<DownloadVideoCover>& downloadVideoCovers);
	bool insert(const DownloadVideoCoverReq& req);
	bool insert(const QList<DownloadVideoCoverReq>& reqs);

	bool remove(const DownloadVideoCoverReq& req);

	QList<DownloadVideoCover> search(const DownloadVideoCoverReq& req);

	int count(const DownloadVideoCoverReq& req);

	int importFromJson(const QString& filePath);
	bool exportToJson(const QString& filePath) const;

private:
	QSharedPointer<DownloadVideoCoverDAO> m_dao;
};