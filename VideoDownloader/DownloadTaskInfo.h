#pragma once

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
	QHash<QString, QUrl> videoDownloadUrls;
	QHash<QString, QUrl> audioDownloadUrls;
	QHash<QString, qint64> videoSizes;
	QHash<QString, qint64> audioSizes;
	QString selectedVideoQuality;
	QString selectedAudioQuality;
	bool partialDownloadSupport;
	DownloadContext* context;
	DownloadContext* audioContext;
	DownloadStatus status;
	DownloadPeriod downloadPeriod;
	QString downloadFilePath;
	QDateTime endTime;
	qint64 fileSize;
	StreamRequest streamRequest;

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
		context = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), videoDownloadUrls["0"]);
		context->fileSize = fileSize;
		context->partialDownloadSupport = partialDownloadSupport;
	}

	// 估计剩余时间
	QString estimatedTimeRemaining() const
	{

	}
};