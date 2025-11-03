#pragma once

#include "VideoDownloadRequest.h"
#include "ModInfo.h"
#include "VideoInfo.h"
#include "DownloadContext.h"

// 下载任务状态
enum DownloadStatus
{
	Queued = 0,
	Downloading,
	Paused,
	Completed,
	Failed
};

class DownloadTaskInfo
{
public:
	QString taskId;
	VideoInfo videoInfo;
	VideoDownloadRequest request;
	StreamRequest streamRequest;
	DownloadStatus status;
	DownloadContext context;
	QDateTime endTime;

	DownloadTaskInfo()
		: status(Queued)
	{
	}

	bool operator==(const DownloadTaskInfo& other) const
	{
		return taskId == other.taskId;
	}

	// 估计剩余时间
	QString estimatedTimeRemaining() const
	{

	}
};