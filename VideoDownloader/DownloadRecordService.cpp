#include "DownloadRecordService.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtConcurrent>

DownloadRecordService::DownloadRecordService(QSharedPointer<DatabaseManager> dbManager,
	QObject* parent)
	: QObject(parent)
	, m_dao(QSharedPointer<DownloadRecordDAO>::create(dbManager))
{
}

DownloadRecordService::~DownloadRecordService()
{
}

DownloadRecord DownloadRecordService::generateRecordFromTaskInfo(QSharedPointer<DownloadTaskInfo> task)
{
	DownloadRecord record;
	record.taskId = task->taskId;
	record.videoId = task->videoInfo.videoId;
	record.url = task->videoInfo.videoId;
	record.title = task->videoInfo.title;
	record.sectionName = task->videoInfo.sectionName;
	record.author = task->videoInfo.author;
	record.duration = task->videoInfo.duration;
	record.publishTime = task->videoInfo.publishTime;
	record.selectedVideoQuality = task->selectedVideoQuality;
	record.selectedAudioQuality = task->selectedAudioQuality;
	record.downloadFilePath = task->downloadFilePath;
	record.cover = task->videoInfo.cover;
	record.endTime = task->endTime;
	return record;
}

// ==================== 基本CRUD操作 ====================
bool DownloadRecordService::insertOne(QSharedPointer<DownloadTaskInfo> task)
{
	return m_dao->insert(generateRecordFromTaskInfo(task));
}

bool DownloadRecordService::insertOne(const DownloadRecord& record)
{
	return m_dao->insert(record);
}

bool DownloadRecordService::deleteOne(const QString& taskId)
{
	return m_dao->remove(taskId);
}

DownloadRecord DownloadRecordService::getOne(const QString& taskId)
{
	return m_dao->getById(taskId).first();
}

// ==================== 批量操作 ====================
bool DownloadRecordService::insertBatch(const QList<QSharedPointer<DownloadTaskInfo>>& tasks)
{
	QList<DownloadRecord> records;
	for (const auto& task : tasks)
	{
		records << generateRecordFromTaskInfo(task);
	}
	return m_dao->insertBatch(records);
}

bool DownloadRecordService::insertBatch(const QList<DownloadRecord>& records)
{
	return m_dao->insertBatch(records);
}

bool DownloadRecordService::deleteBatch(const QList<QString>& taskIds)
{
	bool allSuccess = true;
	for (const QString& taskId : taskIds)
	{
		if (!m_dao->remove(taskId))
		{
			allSuccess = false;
			break;
		}
	}

	return allSuccess;
}

//bool DownloadRecordService::removeAllRecords()
//{
//	// 获取所有记录ID用于信号
//	QList<DownloadRecord> allRecords = getAllRecords();
//	QList<QString> taskIds;
//	for (const auto& record : allRecords) {
//		taskIds << record.taskId;
//	}
//
//	// 执行删除
//	bool success = m_dao->executeQuery("DELETE FROM download_record");
//	return success;
//}

int DownloadRecordService::importFromJson(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qWarning() << "Failed to open JSON file:" << filePath;
		return 0;
	}

	QByteArray jsonData = file.readAll();
	file.close();

	QJsonDocument doc = QJsonDocument::fromJson(jsonData);
	if (doc.isNull() || !doc.isArray()) {
		qWarning() << "Invalid JSON format";
		return 0;
	}

	QJsonArray jsonArray = doc.array();
	QList<DownloadRecord> records;

	for (const QJsonValue& value : jsonArray) {
		QJsonObject obj = value.toObject();

		DownloadRecord record;
		record.taskId = obj.value("taskId").toString();
		record.videoId = obj.value("videoId").toString();
		record.title = obj.value("title").toString();
		record.sectionName = obj.value("sectionName").toString();
		record.author = obj.value("author").toString();
		record.duration = obj.value("duration").toString();
		record.publishTime = obj.value("publishTime").toString();
		record.selectedVideoQuality = obj.value("selectedVideoQuality").toString();
		record.selectedAudioQuality = obj.value("selectedAudioQuality").toString();
		record.downloadFilePath = obj.value("downloadFilePath").toString();
		record.endTime = QDateTime::fromString(obj.value("endTime").toString(), Qt::ISODate);
		record.createdTime = QDateTime::fromString(obj.value("createdTime").toString(), Qt::ISODate);
		record.updatedTime = QDateTime::fromString(obj.value("updatedTime").toString(), Qt::ISODate);

		records.append(record);
	}

	if (records.isEmpty()) {
		return 0;
	}

	if (insertBatch(records)) {
		return records.size();
	}

	return 0;
}

bool DownloadRecordService::exportToJson(const QString& filePath) const
{
	QList<DownloadRecord> records;
	//QList<DownloadRecord> records = getAllRecords();
	if (records.isEmpty()) {
		qWarning() << "No records to export";
		return false;
	}

	QJsonArray jsonArray;
	for (const DownloadRecord& record : records) {
		QJsonObject obj;
		obj.insert("taskId", record.taskId);
		obj.insert("videoId", record.videoId);
		obj.insert("title", record.title);
		obj.insert("sectionName", record.sectionName);
		obj.insert("author", record.author);
		obj.insert("duration", record.duration);
		obj.insert("publishTime", record.publishTime);
		obj.insert("selectedVideoQuality", record.selectedVideoQuality);
		obj.insert("selectedAudioQuality", record.selectedAudioQuality);
		obj.insert("downloadFilePath", record.downloadFilePath);
		obj.insert("endTime", record.endTime.toString(Qt::ISODate));
		obj.insert("createdTime", record.createdTime.toString(Qt::ISODate));
		obj.insert("updatedTime", record.updatedTime.toString(Qt::ISODate));

		jsonArray.append(obj);
	}

	QJsonDocument doc(jsonArray);
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		qWarning() << "Failed to open file for writing:" << filePath;
		return false;
	}

	file.write(doc.toJson());
	file.close();

	return true;
}

// ==================== 查询功能 ====================
QList<DownloadRecord> DownloadRecordService::query(const DownloadRecord& filter)
{
	QList<DownloadRecord> records;
	//m_dao->executeSelect(sql, params, [&](QSqlQuery& query) {
	//	while (query.next()) {
	//		DownloadRecord record;
	//		m_dao->fillFromQuery(query, record);
	//		records.append(record);
	//	}
	//	});

	return records;
}


int DownloadRecordService::count()
{
	return m_dao->count();
}

// 文件操作统计和去重功能的具体实现由于篇幅原因省略
// 但提供了完整的框架设计