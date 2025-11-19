#include "DownloadCardModel.h"

#include "DownloadTaskInfo.h"
#include "Instrumentor.h"

DownloadCardModel::DownloadCardModel(QObject* parent)
	: QObject(parent)
{
}

DownloadCardModel::DownloadCardModel(QSharedPointer<DownloadTaskInfo> taskInfo, QObject* parent)
	: QObject(parent)
{
	BENCHMARKING_FUNCTION();
	fromDownloadTaskInfo(taskInfo);
}

void DownloadCardModel::setTaskId(const QString& taskId)
{
	m_taskId = taskId;
}

void DownloadCardModel::setTitle(const QString& title)
{
	m_title = title;
}

void DownloadCardModel::setCoverUrl(const QUrl& coverUrl)
{
	m_coverUrl = coverUrl;
}

void DownloadCardModel::setCover(const QByteArray& cover)
{
	m_cover = cover;
}

void DownloadCardModel::setDuration(const QString& duration)
{
	m_duration = duration;
}

void DownloadCardModel::setPublishTime(const QString& publishTime)
{
	m_publishTime = publishTime;
}

void DownloadCardModel::setPublisher(const QString& publisher)
{
	m_publisher = publisher;
}

void DownloadCardModel::setVideoSize(qint64 videoSize)
{
	m_videoSize = videoSize;
	emit videoSizeChanged();
}

void DownloadCardModel::setAudioSize(qint64 audioSize)
{
	m_audioSize = audioSize;
	emit audioSizeChanged();
}

void DownloadCardModel::setState(DownloadCardState state)
{
	m_state = state;
}

void DownloadCardModel::setProgressInfo(const QString& progressInfo)
{
	m_progressInfo = progressInfo;
}

void DownloadCardModel::setProgress(int progress)
{
	m_progress = progress;
	emit progressChanged();
}

void DownloadCardModel::setDownloadSpeed(const QString& downloadSpeed)
{
	m_downloadSpeed = downloadSpeed;
	emit downloadSpeedChanged();
}

void DownloadCardModel::setFilePath(const QString& filePath)
{
	m_filePath = filePath;
	emit filePathChanged();
}

void DownloadCardModel::setVideoQuality(const QString& quality)
{
	if (m_videoQuality != quality) {
		m_videoQuality = quality;
		emit videoQualityChanged();
	}
}

void DownloadCardModel::setAudioQuality(const QString& quality)
{
	m_audioQuality = quality;
}

void DownloadCardModel::fromDownloadTaskInfo(QSharedPointer<DownloadTaskInfo> taskInfo)
{
	m_taskId = taskInfo->taskId;
	m_title = taskInfo->videoInfo.title;
	m_coverUrl = taskInfo->videoInfo.coverUrl;
	m_cover = taskInfo->videoInfo.cover;
	m_duration = taskInfo->videoInfo.duration;
	m_publishTime = StringUtil::formatDateTime(QDateTime::fromSecsSinceEpoch(taskInfo->videoInfo.publishTime.toLongLong()));
	m_publisher = taskInfo->videoInfo.author;
	m_progress = 0;
	m_downloadSpeed = 0;

	// 根据状态设置卡片状态
	switch (taskInfo->status)
	{
	case DownloadStatus::Queued:
		m_state = DownloadCardState::Pending;
		break;
	case DownloadStatus::Downloading:
		m_state = DownloadCardState::Downloading;
		break;
	case DownloadStatus::Completed:
		m_state = DownloadCardState::Downloaded;
		break;
	case DownloadStatus::Failed:
		m_state = DownloadCardState::Error;
		break;
	case DownloadStatus::Paused:
		m_state = DownloadCardState::Downloading; // 暂停状态也显示为下载中，但按钮显示为继续
		break;
	default:
		m_state = DownloadCardState::Pending;
		break;
	}
}