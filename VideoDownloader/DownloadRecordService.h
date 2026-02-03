//#pragma once
//
//#include "DownloadRecord.h"
//
//class DatabaseManager;
//class QueryWrapper;
//class DownloadRecordDAO;
//class DownloadTaskInfo;
//
//typedef DownloadTaskInfo DownloadRecordReq;
//
//class DownloadRecordService
//{
//public:
//	explicit DownloadRecordService(QSharedPointer<DatabaseManager> dbManager);
//	~DownloadRecordService();
//
//	DownloadRecord generateFromReq(const DownloadRecordReq& req);
//
//	bool insert(const DownloadRecord& downloadRecord);
//	bool insert(const QList<DownloadRecord>& downloadRecords);
//	bool insert(const DownloadRecordReq& req);
//	bool insert(const QList<DownloadRecordReq>& reqs);
//
//	bool remove(const DownloadRecordReq& req);
//
//	QList<DownloadRecord> search(const DownloadRecordReq& req);
//
//	int count(const DownloadRecordReq& req);
//
//	int importFromJson(const QString& filePath);
//	bool exportToJson(const QString& filePath) const;
//
//private:
//	DownloadRecordDAO* m_dao;
//};