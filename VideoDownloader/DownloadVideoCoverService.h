#pragma once

#include "DownloadVideoCover.h"

class DatabaseManager;
class QueryWrapper;
class DownloadVideoCoverDAO;
class DownloadTaskInfo;

typedef DownloadTaskInfo DownloadVideoCoverReq;

class DownloadVideoCoverService
{
public:
	explicit DownloadVideoCoverService(QSharedPointer<DatabaseManager> dbManager);
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
	DownloadVideoCoverDAO* m_dao;
};