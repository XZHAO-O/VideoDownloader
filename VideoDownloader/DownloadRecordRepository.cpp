#include "DownloadRecordRepository.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>

#include "LogSystem.h"

class DownloadRecordRepository::Impl {
public:
	QString storagePath;
	QMap<QString, DownloadRecord> records;

	bool loadFromFile() {
		QFile file(storagePath + "/downloads.json");
		if (!file.exists()) {
			LOG_INFO("DownloadRecord", "No existing download records found");
			return true;
		}

		if (!file.open(QIODevice::ReadOnly)) {
			LOG_ERROR("DownloadRecord", "Failed to open download records file: " + file.errorString());
			return false;
		}

		try {
			QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
			QJsonObject root = doc.object();

			if (root["version"].toString() != "1.0") {
				LOG_WARN("DownloadRecord", "Unsupported download records version");
				return false;
			}

			QJsonArray recordsArray = root["records"].toArray();
			for (const auto& recordValue : recordsArray) {
				DownloadRecord record = DownloadRecord::fromJson(recordValue.toObject());
				if (record.isValid()) {
					records[record.id] = record;
				}
			}

			LOG_INFO("DownloadRecord", QString("Loaded %1 download records").arg(records.size()));
			return true;

		}
		catch (const std::exception& e) {
			LOG_ERROR("DownloadRecord", "Failed to parse download records: " + QString(e.what()));
			return false;
		}
	}

	bool saveToFile() {
		QJsonArray recordsArray;
		for (const auto& record : records) {
			recordsArray.append(record.toJson());
		}

		QJsonObject root;
		root["version"] = "1.0";
		root["records"] = recordsArray;

		QFile file(storagePath + "/downloads.json");
		if (!file.open(QIODevice::WriteOnly)) {
			LOG_ERROR("DownloadRecord", "Failed to save download records: " + file.errorString());
			return false;
		}

		file.write(QJsonDocument(root).toJson());
		file.close();

		LOG_DEBUG("DownloadRecord", QString("Saved %1 download records").arg(records.size()));
		return true;
	}
};

DownloadRecordRepository::DownloadRecordRepository(const QString& storagePath, QObject* parent)
	: QObject(parent)
	, d(new Impl)
{
	d->storagePath = storagePath;

	// 确保目录存在
	QDir dir(storagePath);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	loadFromFile();
}

DownloadRecordRepository::~DownloadRecordRepository()
{
	saveToFile();
}

bool DownloadRecordRepository::addRecord(const DownloadRecord& record)
{
	if (!record.isValid()) {
		LOG_ERROR("DownloadRecord", "Attempted to add invalid record");
		return false;
	}

	if (d->records.contains(record.id)) {
		LOG_WARN("DownloadRecord", "Record already exists: " + record.id);
		return false;
	}

	d->records[record.id] = record;
	emit recordAdded(record);

	LOG_INFO("DownloadRecord", "Added download record: " + record.id);
	return saveToFile();
}

bool DownloadRecordRepository::updateRecord(const DownloadRecord& record)
{
	if (!d->records.contains(record.id)) {
		LOG_WARN("DownloadRecord", "Record not found for update: " + record.id);
		return false;
	}

	d->records[record.id] = record;
	emit recordUpdated(record);

	LOG_INFO("DownloadRecord", "Updated download record: " + record.id);
	return saveToFile();
}

bool DownloadRecordRepository::removeRecord(const QString& recordId)
{
	if (!d->records.contains(recordId)) {
		LOG_WARN("DownloadRecord", "Record not found for removal: " + recordId);
		return false;
	}

	d->records.remove(recordId);
	emit recordRemoved(recordId);

	LOG_INFO("DownloadRecord", "Removed download record: " + recordId);
	return saveToFile();
}

DownloadRecord DownloadRecordRepository::getRecord(const QString& recordId) const
{
	return d->records.value(recordId, DownloadRecord());
}

QList<DownloadRecord> DownloadRecordRepository::getAllRecords() const
{
	return d->records.values();
}

QList<DownloadRecord> DownloadRecordRepository::getRecordsByPlatform(const QString& platformId) const
{
	QList<DownloadRecord> result;
	for (const auto& record : d->records) {
		if (record.platformId == platformId) {
			result.append(record);
		}
	}
	return result;
}

QList<DownloadRecord> DownloadRecordRepository::getRecordsByDateRange(const QDateTime& start, const QDateTime& end) const
{
	QList<DownloadRecord> result;
	for (const auto& record : d->records) {
		if (record.downloadTime >= start && record.downloadTime <= end) {
			result.append(record);
		}
	}
	return result;
}

QList<DownloadRecord> DownloadRecordRepository::getSuccessfulRecords() const
{
	QList<DownloadRecord> result;
	for (const auto& record : d->records) {
		if (record.success) {
			result.append(record);
		}
	}
	return result;
}

QList<DownloadRecord> DownloadRecordRepository::getFailedRecords() const
{
	QList<DownloadRecord> result;
	for (const auto& record : d->records) {
		if (!record.success) {
			result.append(record);
		}
	}
	return result;
}

int DownloadRecordRepository::getTotalCount() const
{
	return d->records.size();
}

qint64 DownloadRecordRepository::getTotalDownloadSize() const
{
	qint64 total = 0;
	for (const auto& record : d->records) {
		if (record.success) {
			total += record.fileSize;
		}
	}
	return total;
}

int DownloadRecordRepository::getSuccessCount() const
{
	return getSuccessfulRecords().size();
}

int DownloadRecordRepository::getFailureCount() const
{
	return getFailedRecords().size();
}

bool DownloadRecordRepository::loadFromFile()
{
	return d->loadFromFile();
}

bool DownloadRecordRepository::saveToFile()
{
	return d->saveToFile();
}

bool DownloadRecordRepository::clearAll()
{
	int count = d->records.size();
	d->records.clear();
	LOG_INFO("DownloadRecord", QString("Cleared all %1 records").arg(count));
	return saveToFile();
}