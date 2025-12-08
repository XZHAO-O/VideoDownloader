#pragma once

#include "DownloadRecord.h"

class RecordManager
{
public:
	// 查找重复记录（基于视频ID）
	QList<QList<DownloadRecord>> findDuplicateRecords();

	// 清理无效记录（文件不存在的记录）
	QList<QString> cleanupInvalidRecords();

	// 清理过时记录（保留最近N天的记录）
	int cleanupOldRecords(int keepDays = 90);

	// 查找缺失文件的记录
	QList<DownloadRecord> findMissingFileRecords();
};
