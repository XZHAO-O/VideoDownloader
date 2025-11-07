#pragma once

#include "VideoDownloadRequest.h"
#include "ModInfo.h"
#include "VideoInfo.h"
#include "DownloadContext.h"

// 下载任务状态
enum class DownloadPeriod
{
	Prepare = 0,
	Video,
	Audio,
	Merge,
};

class DownloadTaskInfo
{
public:
	QString taskId;
	VideoInfo videoInfo;
	VideoDownloadRequest request;
	StreamRequest streamRequest;
	DownloadStatus status;
	DownloadPeriod downloadPeriod;
	DownloadContext context;
	QDateTime endTime;

	DownloadTaskInfo()
		: status(DownloadStatus::Queued)
		, downloadPeriod(DownloadPeriod::Prepare)
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