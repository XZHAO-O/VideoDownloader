#include "DownloadCardModel.h"

#include "DownloadTaskInfo.h"
#include "Instrumentor.h"

DownloadCardModel::DownloadCardModel(QObject* parent)
	: QObject(parent)
{
}

DownloadCardModel::DownloadCardModel(const DownloadTaskInfo& taskInfo, QObject* parent)
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
	emit coverUrlChanged();
}

void DownloadCardModel::setCover(const QByteArray& cover)
{
	m_cover = cover;
}

void DownloadCardModel::setDuration(const QString& duration)
{
	m_duration = duration;
}

void DownloadCardModel::setPublishTime(const QDateTime& publishTime)
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

void DownloadCardModel::setProgress(int progress)
{
	m_progress = progress;
	emit progressChanged();
}

void DownloadCardModel::setDownloadSpeed(qint64 downloadSpeed)
{
	m_downloadSpeed = downloadSpeed;
	emit downloadSpeedChanged();
}

void DownloadCardModel::setFilePath(const QString& filePath)
{
	m_filePath = filePath;
	emit filePathChanged();
}

void DownloadCardModel::setVideoQuality(VideoQualityLevel quality)
{
	if (m_videoQuality != quality) {
		m_videoQuality = quality;
		emit videoQualityChanged();
	}
}

void DownloadCardModel::setAudioQuality(AudioQualityLevel quality)
{
	m_audioQuality = quality;
}

void DownloadCardModel::setDownloadedSize(qint64 downloadedSize)
{
	m_downloadedSize = downloadedSize;
}

void DownloadCardModel::setDownloadSize(qint64 totalSize)
{
	m_downloadSize = totalSize;
}

QString DownloadCardModel::formattedVideoSize() const
{
	return DownloadTaskInfo::formatFileSize(m_videoSize);
}

QString DownloadCardModel::formattedAudioSize() const
{
	return DownloadTaskInfo::formatFileSize(m_audioSize);
}

QString DownloadCardModel::formattedDownloadSpeed() const
{
	if (m_downloadSpeed <= 0) return "0 B/s";

	const qint64 KB = 1024;
	const qint64 MB = KB * 1024;

	if (m_downloadSpeed >= MB)
		return QString("%1 MB/s").arg(QString::number(m_downloadSpeed / static_cast<double>(MB), 'f', 1));
	else if (m_downloadSpeed >= KB)
		return QString("%1 KB/s").arg(m_downloadSpeed / KB);
	else
		return QString("%1 B/s").arg(m_downloadSpeed);
}

QString DownloadCardModel::formattedPublishTime() const
{
	if (!m_publishTime.isValid()) return "";

	QDateTime now = QDateTime::currentDateTime();
	qint64 days = m_publishTime.daysTo(now);

	if (days == 0) {
		return m_publishTime.toString("今天 hh:mm");
	}
	else if (days == 1) {
		return m_publishTime.toString("昨天 hh:mm");
	}
	else if (days < 7) {
		return QString("%1天前").arg(days);
	}
	else {
		return m_publishTime.toString("yyyy-MM-dd");
	}
}

QString DownloadCardModel::formattedDuration() const
{
	return m_duration; // 假设已经是格式化好的时长
}

void DownloadCardModel::fromDownloadTaskInfo(const DownloadTaskInfo& taskInfo)
{
	m_taskId = taskInfo.taskId;
	m_title = taskInfo.videoInfo.title;
	m_coverUrl = taskInfo.videoInfo.thumbnailUrl;
	m_cover = taskInfo.videoInfo.cover;
	m_duration = taskInfo.videoInfo.duration;
	m_publishTime = taskInfo.videoInfo.uploadDate;
	m_publisher = taskInfo.videoInfo.author;
	m_progress = taskInfo.progressPercentage;
	m_downloadSpeed = taskInfo.downloadSpeed;

	// 根据状态设置卡片状态
	switch (taskInfo.status)
	{
	case Queued:
		m_state = DownloadCardState::Pending;
		break;
	case Downloading:
		m_state = DownloadCardState::Downloading;
		break;
	case Completed:
		m_state = DownloadCardState::Downloaded;
		break;
	case Failed:
		m_state = DownloadCardState::Error;
		break;
	case Paused:
		m_state = DownloadCardState::Downloading; // 暂停状态也显示为下载中，但按钮显示为继续
		break;
	default:
		m_state = DownloadCardState::Pending;
		break;
	}
}