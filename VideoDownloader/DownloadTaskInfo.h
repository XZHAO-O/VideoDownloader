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
	QHash<QString, StreamInfo> videoStreamInfo;
	QHash<QString, StreamInfo> audioStreamInfo;
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
		selectedVideoQuality = videoStreamInfo.keys()[0];
		context = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), videoStreamInfo[selectedVideoQuality].url);
		context->fileSize = videoStreamInfo[selectedVideoQuality].fileSize;
		context->partialDownloadSupport = true;
	}

	void createAudioContext()
	{
		selectedAudioQuality = audioStreamInfo.keys()[0];
		audioContext = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title), videoStreamInfo[selectedAudioQuality].url);
		audioContext->fileSize = videoStreamInfo[selectedAudioQuality].fileSize;
		audioContext->partialDownloadSupport = true;
	}
};