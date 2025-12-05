#pragma once

#include <QObject>
#include <QSharedPointer>

#include "DownloadRecordDAO.h"
#include "DownloadTaskInfo.h"

class DownloadRecordService : public QObject
{
	Q_OBJECT

public:
	explicit DownloadRecordService(QSharedPointer<DatabaseManager> dbManager,
		QObject* parent = nullptr);
	~DownloadRecordService();

	DownloadRecord generateRecordFromTaskInfo(QSharedPointer<DownloadTaskInfo> task);

	// ==================== 基本CRUD操作 ====================
	bool insertOne(QSharedPointer<DownloadTaskInfo> task);
	bool insertOne(const DownloadRecord& record);
	bool deleteOne(const QString& taskId);
	DownloadRecord getOne(const QString& taskId);

	// ==================== 批量操作 ====================
	bool insertBatch(const QList<QSharedPointer<DownloadTaskInfo>>& tasks);
	bool insertBatch(const QList<DownloadRecord>& records);
	bool deleteBatch(const QList<QString>& taskIds);

	int importFromJson(const QString& filePath);
	bool exportToJson(const QString& filePath) const;

	// ==================== 查询功能 ====================
	// 组合查询
	QList<DownloadRecord> query(const DownloadRecord& filter);

	// 快速统计
	int count();

	// ==================== 去重与清理功能 ====================
	// 查找重复记录（基于视频ID）
	QList<QList<DownloadRecord>> findDuplicateRecords();

	// 清理无效记录（文件不存在的记录）
	QList<QString> cleanupInvalidRecords();

	// 清理过时记录（保留最近N天的记录）
	int cleanupOldRecords(int keepDays = 90);

	// 查找缺失文件的记录
	QList<DownloadRecord> findMissingFileRecords();

private:
	// 文件操作辅助方法、集成到文件工具中
	//qint64 getFileSize(const QString& filePath) const;
	//bool fileExists(const QString& filePath) const;

private:
	QSharedPointer<DownloadRecordDAO> m_dao;
};