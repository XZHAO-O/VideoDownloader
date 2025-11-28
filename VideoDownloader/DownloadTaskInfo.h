#pragma once

#include "VideoInfo.h"
#include "DownloadContext.h"
#include "OrderedQHash.h"

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
	Merged,
	Separated,
};

class DownloadTaskInfo
{
public:
	QString taskId;
	VideoInfo videoInfo;
	OrderedQHash<QString, StreamInfo> videoStreamInfo;
	OrderedQHash<QString, StreamInfo> audioStreamInfo;
	QString selectedVideoQuality;
	QString selectedAudioQuality;
	DownloadContext* videoContext;
	DownloadContext* audioContext;
	DownloadFormat downloadFormat;
	DownloadStatus status;
	DownloadPeriod downloadPeriod;
	bool partialDownloadSupport;
	QString downloadFilePath;
	QDateTime endTime;

	DownloadTaskInfo()
		: videoContext(nullptr)
		, audioContext(nullptr)
		, downloadFormat(DownloadFormat::Merged)
		, status(DownloadStatus::Queued)
		, downloadPeriod(DownloadPeriod::Prepare)
	{
	}

	~DownloadTaskInfo()
	{
		if (videoContext)
		{
			delete videoContext;
			videoContext = nullptr;
		}
		if (audioContext)
		{
			delete audioContext;
			audioContext = nullptr;
		}
	}

	bool operator==(const DownloadTaskInfo& other) const
	{
		return taskId == other.taskId;
	}

	bool isCompleted() const
	{
		switch (downloadFormat)
		{
		case DownloadFormat::Separated:
			return videoContext && videoContext->downloadStatus == DownloadStatus::Completed &&
				audioContext && audioContext->downloadStatus == DownloadStatus::Completed;
		case DownloadFormat::AudioOnly:
			return audioContext && audioContext->downloadStatus == DownloadStatus::Completed;
		case DownloadFormat::VideoOnly:
		case DownloadFormat::Merged:
			return videoContext && videoContext->downloadStatus == DownloadStatus::Completed;
		}
		return false;
	}

	bool isFailed() const
	{
		switch (downloadFormat)
		{
		case DownloadFormat::Separated:
			return (videoContext && videoContext->downloadStatus == DownloadStatus::Failed) ||
				(audioContext && audioContext->downloadStatus == DownloadStatus::Failed);
		case DownloadFormat::AudioOnly:
			return audioContext && audioContext->downloadStatus == DownloadStatus::Failed;
		case DownloadFormat::VideoOnly:
		case DownloadFormat::Merged:
			return videoContext && videoContext->downloadStatus == DownloadStatus::Failed;
		}
		return false;
	}

	void createContext()
	{
		switch (videoInfo.streamType)
		{
		case StreamType::AVSeparate:
		{
			switch (downloadFormat)
			{
			case DownloadFormat::Separated:
				createVideoContext();
				createAudioContext();
				return;
			case DownloadFormat::AudioOnly:
				createAudioContext();
				return;
			case DownloadFormat::VideoOnly:
			case DownloadFormat::Merged:
				createVideoContext();
				return;
			}
		}
		case StreamType::AVMerged:
		{
			createVideoContext();
			break;
		}
		}
	}

	void startDownload(QSharedPointer<NetworkManager> networkManager, Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		switch (downloadFormat)
		{
		case DownloadFormat::AudioOnly:
			if (audioContext)
			{
				QMetaObject::invokeMethod(audioContext, [this, networkManager]() {
					audioContext->startDownload(networkManager);
					}, connectionType);
			}
			return;
		case DownloadFormat::Separated:
			downloadPeriod = DownloadPeriod::Video;
			if (videoContext)
			{
				QMetaObject::invokeMethod(videoContext, [this, networkManager]() {
					videoContext->startDownload(networkManager);
					}, connectionType);
			}
			return;
		case DownloadFormat::VideoOnly:
		case DownloadFormat::Merged:
			downloadPeriod = DownloadPeriod::Video;
			if (videoContext)
			{
				QMetaObject::invokeMethod(videoContext, [this, networkManager]() {
					videoContext->startDownload(networkManager);
					}, connectionType);
			}
			return;
		}
	}

	void pauseDownload(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		switch (downloadFormat)
		{
		case DownloadFormat::Merged:
		case DownloadFormat::VideoOnly:
			pauseVideoContext(connectionType);
			break;

		case DownloadFormat::AudioOnly:
			pauseAudioContext(connectionType);
			break;
		case DownloadFormat::Separated:
			switch (downloadPeriod)
			{
			case DownloadPeriod::Video:
				pauseVideoContext(connectionType);
				break;
			case DownloadPeriod::Audio:
				pauseAudioContext(connectionType);
				break;
			}
			break;
		}
	}

	void cancelDownload(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		switch (downloadFormat)
		{
		case DownloadFormat::Merged:
		case DownloadFormat::VideoOnly:
			cancelVideoContext(connectionType);
			break;

		case DownloadFormat::AudioOnly:
			cancelAudioContext(connectionType);
			break;
		case DownloadFormat::Separated:
			cancelVideoContext(connectionType);
			cancelAudioContext(connectionType);
			break;
		}
	}

	template <typename T>
	void disconnectContext(T target)
	{
		switch (downloadFormat)
		{
		case DownloadFormat::Merged:
		case DownloadFormat::VideoOnly:
			if (videoContext)
				QObject::disconnect(videoContext, nullptr, target, nullptr);
			break;

		case DownloadFormat::Separated:
			if (videoContext)
				QObject::disconnect(videoContext, nullptr, target, nullptr);
		case DownloadFormat::AudioOnly:
			if (audioContext)
				QObject::disconnect(audioContext, nullptr, target, nullptr);
			break;
		}
	}

	void formatDownloadInfo(int& progress, QString& progressInfo, QString& downloadSpeed)
	{
		DownloadContext* downloadContext = nullptr;
		switch (downloadFormat)
		{
		case DownloadFormat::Merged:
		case DownloadFormat::VideoOnly:
			downloadContext = videoContext;
			break;

		case DownloadFormat::Separated:
		{
			switch (downloadPeriod)
			{
			case DownloadPeriod::Video:
				downloadContext = videoContext;
				break;
			case DownloadPeriod::Audio:
				downloadContext = audioContext;
				break;
			}
			break;
		}
		case DownloadFormat::AudioOnly:
			downloadContext = audioContext;
			break;
		}
		if (!downloadContext)
			return;
		qint64 downloadedBytes = downloadContext->downloadedTotalSize;
		progressInfo = StringUtil::formatDownloadProgress(downloadedBytes, downloadContext->fileSize);
		progress = downloadedBytes * 100 / downloadContext->fileSize;
		downloadSpeed = StringUtil::formatDownloadSpeed(downloadedBytes - downloadContext->progressedSize);
		downloadContext->progressedSize = downloadedBytes;
	}

private:
	void createVideoContext()
	{
		if (!videoStreamInfo.isEmpty())
		{
			videoContext = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title) + ".mp4", videoStreamInfo[selectedVideoQuality].url);
			videoContext->fileSize = videoStreamInfo[selectedVideoQuality].fileSize;
		}
	}

	void createAudioContext()
	{
		if (!audioStreamInfo.isEmpty())
		{
			audioContext = new DownloadContext("E:/CProject/" + StringUtil::formatFileName(videoInfo.title) + "_audio" + ".aac", audioStreamInfo[selectedAudioQuality].url);
			audioContext->fileSize = audioStreamInfo[selectedAudioQuality].fileSize;
		}
	}

	void pauseVideoContext(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		if (videoContext)
		{
			QMetaObject::invokeMethod(videoContext, [this]() {
				videoContext->pauseDownload();
				}, connectionType);
		}
	}

	void pauseAudioContext(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		if (audioContext)
		{
			QMetaObject::invokeMethod(audioContext, [this]() {
				audioContext->pauseDownload();
				}, connectionType);
		}
	}

	void cancelVideoContext(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		if (videoContext)
		{
			QMetaObject::invokeMethod(videoContext, [this]() {
				videoContext->cancelDownload();
				}, connectionType);
		}
	}

	void cancelAudioContext(Qt::ConnectionType connectionType = Qt::QueuedConnection)
	{
		if (audioContext)
		{
			QMetaObject::invokeMethod(audioContext, [this]() {
				audioContext->cancelDownload();
				}, connectionType);
		}
	}
};