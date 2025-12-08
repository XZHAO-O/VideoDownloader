#pragma once

#include <QObject>
#include <QSharedPointer>

#include "DownloadRecordDAO.h"
#include "DownloadTaskInfo.h"

class DownloadRecordReq;

class DownloadRecordService : public QObject
{
	Q_OBJECT

public:
	explicit DownloadRecordService(QSharedPointer<DatabaseManager> dbManager,
		QObject* parent = nullptr);
	~DownloadRecordService();

	DownloadRecord generateFromReq(QSharedPointer<DownloadTaskInfo> task);

	bool insert(const QSharedPointer<DownloadTaskInfo>& task);
	bool insert(const QList<QSharedPointer<DownloadTaskInfo>>& task);
	bool insert(const DownloadRecord& downloadrecord);
	bool insert(const QList<DownloadRecord>& downloadrecords);

	bool remove(const QSharedPointer<DownloadTaskInfo>& task);
	bool remove(const QList<QSharedPointer<DownloadTaskInfo>>& tasks);

	QList<DownloadRecord> search(const DownloadRecordReq& req);

	int count(const QueryWrapper& wrapper);

	int importFromJson(const QString& filePath);
	bool exportToJson(const QString& filePath) const;

private:
	QSharedPointer<DownloadRecordDAO> m_dao;
};