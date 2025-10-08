#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include "DownloadRecord.h"

class DownloadRecordRepository : public QObject
{
	Q_OBJECT

public:
	explicit DownloadRecordRepository(const QString& storagePath, QObject* parent = nullptr);
	~DownloadRecordRepository();

	// 基本操作
	bool addRecord(const DownloadRecord& record);
	bool updateRecord(const DownloadRecord& record);
	bool removeRecord(const QString& recordId);
	DownloadRecord getRecord(const QString& recordId) const;
	QList<DownloadRecord> getAllRecords() const;

	// 查询操作
	QList<DownloadRecord> getRecordsByPlatform(const QString& platformId) const;
	QList<DownloadRecord> getRecordsByDateRange(const QDateTime& start, const QDateTime& end) const;
	QList<DownloadRecord> getSuccessfulRecords() const;
	QList<DownloadRecord> getFailedRecords() const;

	// 统计信息
	int getTotalCount() const;
	qint64 getTotalDownloadSize() const;
	int getSuccessCount() const;
	int getFailureCount() const;

	// 持久化
	bool loadFromFile();
	bool saveToFile();
	bool clearAll();

signals:
	void recordAdded(const DownloadRecord& record);
	void recordUpdated(const DownloadRecord& record);
	void recordRemoved(const QString& recordId);

private:
	class Impl;
	QScopedPointer<Impl> d;
};