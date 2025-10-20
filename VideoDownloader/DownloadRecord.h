#pragma once

#include <QJsonObject>

struct DownloadRecord {
	QString id;
	QString taskId;
	QString videoTitle;
	QString platformId;
	QString filePath;
	qint64 fileSize;
	QDateTime downloadTime;
	QString videoUrl;
	QString videoQuality;
	QString audioQuality;
	QString format;
	bool success;
	QString errorMessage;

	// 生成唯一记录ID
	static QString generateRecordId() {
		return QString("record_%1_%2")
			.arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz"))
			.arg(QUuid::createUuid().toString().mid(1, 8));
	}

	// 序列化方法
	QJsonObject toJson() const {
		QJsonObject obj;
		obj["id"] = id;
		obj["taskId"] = taskId;
		obj["videoTitle"] = videoTitle;
		obj["platformId"] = platformId;
		obj["filePath"] = filePath;
		obj["fileSize"] = fileSize;
		obj["downloadTime"] = downloadTime.toString(Qt::ISODate);
		obj["videoUrl"] = videoUrl;
		obj["videoQuality"] = videoQuality;
		obj["audioQuality"] = audioQuality;
		obj["format"] = format;
		obj["success"] = success;
		obj["errorMessage"] = errorMessage;
		return obj;
	}

	// 反序列化方法
	static DownloadRecord fromJson(const QJsonObject& obj) {
		DownloadRecord record;
		record.id = obj["id"].toString();
		record.taskId = obj["taskId"].toString();
		record.videoTitle = obj["videoTitle"].toString();
		record.platformId = obj["platformId"].toString();
		record.filePath = obj["filePath"].toString();
		record.fileSize = obj["fileSize"].toVariant().toLongLong();
		record.downloadTime = QDateTime::fromString(obj["downloadTime"].toString(), Qt::ISODate);
		record.videoUrl = obj["videoUrl"].toString();
		record.videoQuality = obj["videoQuality"].toString();
		record.audioQuality = obj["audioQuality"].toString();
		record.format = obj["format"].toString();
		record.success = obj["success"].toBool();
		record.errorMessage = obj["errorMessage"].toString();
		return record;
	}

	bool isValid() const {
		return !id.isEmpty() && !taskId.isEmpty();
	}
};