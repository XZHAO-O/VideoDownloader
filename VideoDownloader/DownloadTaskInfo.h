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
	DownloadContext* context;
	DownloadStatus status;
	DownloadPeriod downloadPeriod;
	QDateTime endTime;
	qint64 fileSize;
	bool partialDownloadSupport;

	DownloadTaskInfo()
		: context(nullptr)
		, status(DownloadStatus::Queued)
		, downloadPeriod(DownloadPeriod::Prepare)
	{
	}

	bool operator==(const DownloadTaskInfo& other) const
	{
		return taskId == other.taskId;
	}

	void createContext()
	{
		context = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), request.videoPlayUrl);
		context->fileSize = fileSize;
		context->partialDownloadSupport = partialDownloadSupport;
	}

	// 估计剩余时间
	QString estimatedTimeRemaining() const
	{

	}
};