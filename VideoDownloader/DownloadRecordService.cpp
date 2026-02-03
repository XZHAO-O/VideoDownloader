//#include "DownloadRecordService.h"
//
//#include <QFile>
//#include <QJsonDocument>
//#include <QJsonArray>
//#include <QJsonObject>
//
//#include "QueryWrapper.h"
//#include "DownloadRecordDAO.h"
//#include "DownloadTaskInfo.h"
//
//DownloadRecordService::DownloadRecordService(QSharedPointer<DatabaseManager> dbManager)
//	: m_dao(new DownloadRecordDAO(dbManager))
//{
//}
//
//DownloadRecordService::~DownloadRecordService()
//{
//	delete m_dao;
//}
//
//DownloadRecord DownloadRecordService::generateFromReq(const DownloadRecordReq& req)
//{
//	DownloadRecord downloadRecord;
//	downloadRecord.taskId = req.taskId;
//	downloadRecord.videoId = req.videoInfo.videoId;
//	downloadRecord.url = req.videoInfo.videoId;
//	downloadRecord.title = req.videoInfo.title;
//	downloadRecord.sectionName = req.videoInfo.sectionName;
//	downloadRecord.author = req.videoInfo.author;
//	downloadRecord.duration = req.videoInfo.duration;
//	downloadRecord.publishTime = req.videoInfo.publishTime;
//	downloadRecord.selectedVideoQuality = req.selectedVideoQuality;
//	downloadRecord.selectedAudioQuality = req.selectedAudioQuality;
//	downloadRecord.downloadFilePath = req.downloadFilePath;
//	downloadRecord.endTime = req.endTime;
//	return downloadRecord;
//}
//
//bool DownloadRecordService::insert(const DownloadRecord& downloadRecord)
//{
//	return m_dao->insert(downloadRecord);
//}
//
//bool DownloadRecordService::insert(const QList<DownloadRecord>& downloadRecords)
//{
//	return m_dao->insertBatch(downloadRecords);
//}
//
//bool DownloadRecordService::insert(const DownloadRecordReq& req)
//{
//	DownloadRecord downloadRecord = generateFromReq(req);
//	return insert(downloadRecord);
//}
//
//bool DownloadRecordService::insert(const QList<DownloadRecordReq>& reqs)
//{
//	QList<DownloadRecord> downloadRecords;
//	downloadRecords.reserve(reqs.size());
//	for (const auto& req : reqs)
//	{
//		downloadRecords.append(generateFromReq(req));
//	}
//	return insert(downloadRecords);
//}
//
//bool DownloadRecordService::remove(const DownloadRecordReq& req)
//{
//	QueryWrapper wrapper;
//	return m_dao->remove(wrapper);
//}
//
//QList<DownloadRecord> DownloadRecordService::search(const DownloadRecordReq& req)
//{
//	QueryWrapper wrapper;
//	return m_dao->list(wrapper);
//}
//
//int DownloadRecordService::count(const DownloadRecordReq& req)
//{
//	QueryWrapper wrapper;
//	return m_dao->count(wrapper);
//}
//
//int DownloadRecordService::importFromJson(const QString& filePath)
//{
//	QFile file(filePath);
//	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//		qWarning() << "Failed to open JSON file:" << filePath;
//		return 0;
//	}
//
//	QByteArray jsonData = file.readAll();
//	file.close();
//
//	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
//	if (doc.isNull() || !doc.isArray()) {
//		qWarning() << "Invalid JSON format";
//		return 0;
//	}
//
//	QJsonArray jsonArray = doc.array();
//	QList<DownloadRecord> records;
//	records.reserve(jsonArray.size());
//	for (const QJsonValue& value : jsonArray) {
//		QJsonObject obj = value.toObject();
//
//		DownloadRecord record;
//		record.taskId = obj.value("taskId").toString();
//		record.videoId = obj.value("videoId").toString();
//		record.url = obj.value("url").toString();
//		record.title = obj.value("title").toString();
//		record.sectionName = obj.value("sectionName").toString();
//		record.author = obj.value("author").toString();
//		record.duration = obj.value("duration").toString();
//		record.publishTime = obj.value("publishTime").toString();
//		record.selectedVideoQuality = obj.value("selectedVideoQuality").toString();
//		record.selectedAudioQuality = obj.value("selectedAudioQuality").toString();
//		record.downloadFilePath = obj.value("downloadFilePath").toString();
//		record.endTime = QDateTime::fromString(obj.value("endTime").toString(), Qt::ISODate);
//		record.createdTime = QDateTime::fromString(obj.value("createdTime").toString(), Qt::ISODate);
//		record.updatedTime = QDateTime::fromString(obj.value("updatedTime").toString(), Qt::ISODate);
//
//		records.append(record);
//	}
//
//	if (records.isEmpty()) {
//		return 0;
//	}
//
//	if (insert(records)) {
//		return records.size();
//	}
//
//	return 0;
//}
//
//bool DownloadRecordService::exportToJson(const QString& filePath) const
//{
//	// 使用空查询条件获取所有记录
//	DownloadRecordReq req;
//	//QList<DownloadRecord> records = search(req);
//	QList<DownloadRecord> records;
//	if (records.isEmpty()) {
//		qWarning() << "No records to export";
//		return false;
//	}
//
//	QJsonArray jsonArray;
//	for (const DownloadRecord& record : records) {
//		QJsonObject obj;
//		obj.insert("taskId", record.taskId);
//		obj.insert("videoId", record.videoId);
//		obj.insert("url", record.url);
//		obj.insert("title", record.title);
//		obj.insert("sectionName", record.sectionName);
//		obj.insert("author", record.author);
//		obj.insert("duration", record.duration);
//		obj.insert("publishTime", record.publishTime);
//		obj.insert("selectedVideoQuality", record.selectedVideoQuality);
//		obj.insert("selectedAudioQuality", record.selectedAudioQuality);
//		obj.insert("downloadFilePath", record.downloadFilePath);
//		obj.insert("endTime", record.endTime.toString(Qt::ISODate));
//		obj.insert("createdTime", record.createdTime.toString(Qt::ISODate));
//		obj.insert("updatedTime", record.updatedTime.toString(Qt::ISODate));
//
//		jsonArray.append(obj);
//	}
//
//	QJsonDocument doc(jsonArray);
//	QFile file(filePath);
//	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//		qWarning() << "Failed to open file for writing:" << filePath;
//		return false;
//	}
//
//	file.write(doc.toJson());
//	file.close();
//
//	return true;
//}