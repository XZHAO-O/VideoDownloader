#include "DownloadVideoCoverService.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "QueryWrapper.h"
#include "DownloadVideoCoverDAO.h"
#include "DownloadTaskInfo.h"

DownloadVideoCoverService::DownloadVideoCoverService(QSharedPointer<DatabaseManager> dbManager,
	QObject* parent)
	: QObject(parent)
	, m_dao(QSharedPointer<DownloadVideoCoverDAO>::create(dbManager))
{
}

DownloadVideoCoverService::~DownloadVideoCoverService()
{
}

DownloadVideoCover DownloadVideoCoverService::generateFromReq(const DownloadVideoCoverReq& req)
{
	DownloadVideoCover downloadVideoCover;
	downloadVideoCover.taskId = req.taskId;
	downloadVideoCover.cover = req.videoInfo.cover;
	return downloadVideoCover;
}

bool DownloadVideoCoverService::insert(const DownloadVideoCover& downloadVideoCover)
{
	return m_dao->insert(downloadVideoCover);
}

bool DownloadVideoCoverService::insert(const QList<DownloadVideoCover>& downloadVideoCovers)
{
	return m_dao->insertBatch(downloadVideoCovers);
}

bool DownloadVideoCoverService::insert(const DownloadVideoCoverReq& req)
{
	DownloadVideoCover downloadVideoCover = generateFromReq(req);
	return insert(downloadVideoCover);
}

bool DownloadVideoCoverService::insert(const QList<DownloadVideoCoverReq>& reqs)
{
	QList<DownloadVideoCover> downloadVideoCovers;
	downloadVideoCovers.reserve(reqs.size());
	for (const auto& req : reqs)
	{
		downloadVideoCovers.append(generateFromReq(req));
	}
	return insert(downloadVideoCovers);
}

bool DownloadVideoCoverService::remove(const DownloadVideoCoverReq& req)
{
	QueryWrapper wrapper;
	return m_dao->remove(wrapper);
}

QList<DownloadVideoCover> DownloadVideoCoverService::search(const DownloadVideoCoverReq& req)
{
	QueryWrapper wrapper;
	return m_dao->list(wrapper);
}

int DownloadVideoCoverService::count(const DownloadVideoCoverReq& req)
{
	QueryWrapper wrapper;
	return m_dao->count(wrapper);
}

int DownloadVideoCoverService::importFromJson(const QString& filePath)
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
	QList<DownloadVideoCover> records;
	records.reserve(jsonArray.size());
	for (const QJsonValue& value : jsonArray) {
		QJsonObject obj = value.toObject();

		DownloadVideoCover record;
		record.taskId = obj.value("taskId").toString();
		record.cover = QByteArray::fromBase64(obj.value("cover").toString().toLatin1());
		record.createdTime = QDateTime::fromString(obj.value("createdTime").toString(), Qt::ISODate);
		record.updatedTime = QDateTime::fromString(obj.value("updatedTime").toString(), Qt::ISODate);

		records.append(record);
	}

	if (records.isEmpty()) {
		return 0;
	}

	if (insert(records)) {
		return records.size();
	}

	return 0;
}

bool DownloadVideoCoverService::exportToJson(const QString& filePath) const
{
	// 使用空查询条件获取所有记录
	QList<DownloadVideoCover> records;
	if (records.isEmpty()) {
		qWarning() << "No records to export";
		return false;
	}

	QJsonArray jsonArray;
	for (const DownloadVideoCover& record : records) {
		QJsonObject obj;
		obj.insert("taskId", record.taskId);
		obj.insert("cover", QString(record.cover.toBase64()));
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