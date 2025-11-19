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
	Merging,
};

// 下载格式
enum class DownloadFormat
{
	VideoOnly = 0,
	AudioOnly,
	Merged
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
	DownloadContext* context;
	DownloadContext* audioContext;
	DownloadFormat downloadFormat;
	DownloadStatus status;
	DownloadPeriod downloadPeriod;
	bool partialDownloadSupport;
	QString downloadFilePath;
	QDateTime endTime;

	DownloadTaskInfo()
		: context(nullptr)
		, audioContext(nullptr)
		, downloadFormat(DownloadFormat::Merged)
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
		switch (videoInfo.streamType)
		{
		case StreamType::AVSeparate:
		{
			switch (downloadFormat)
			{
			case DownloadFormat::VideoOnly:
			{
				createVideoContext();
				return;
			}
			case DownloadFormat::AudioOnly:
				createAudioContext();
				return;
			}
		}
		case StreamType::AVMerged:
		{
			createVideoContext();
			createAudioContext();
			break;
		}
		}
	}

private:
	void createVideoContext()
	{
		context = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), audioDownloadUrls["0"]);
		context->fileSize = videoSizes["0"];
		context->partialDownloadSupport = partialDownloadSupport;
	}

	void createAudioContext()
	{
		audioContext = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), audioDownloadUrls["0"]);
		audioContext->fileSize = audioSizes["0"];
		audioContext->partialDownloadSupport = partialDownloadSupport;
	}
};