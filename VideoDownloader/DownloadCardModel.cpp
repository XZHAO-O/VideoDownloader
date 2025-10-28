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
	if (m_taskId != taskId) {
		m_taskId = taskId;
		emit taskIdChanged();
	}
}

void DownloadCardModel::setTitle(const QString& title)
{
	if (m_title != title) {
		m_title = title;
		emit titleChanged();
	}
}

void DownloadCardModel::setCoverUrl(const QUrl& coverUrl)
{
	if (m_coverUrl != coverUrl) {
		m_coverUrl = coverUrl;
		emit coverUrlChanged();
	}
}

void DownloadCardModel::setCover(const QByteArray& cover)
{
	if (m_cover != cover) {
		m_cover = cover;
		emit coverChanged();
	}
}

void DownloadCardModel::setDuration(const QString& duration)
{
	if (m_duration != duration) {
		m_duration = duration;
		emit durationChanged();
	}
}

void DownloadCardModel::setPublishTime(const QDateTime& publishTime)
{
	if (m_publishTime != publishTime) {
		m_publishTime = publishTime;
		emit publishTimeChanged();
	}
}

void DownloadCardModel::setPublisher(const QString& publisher)
{
	if (m_publisher != publisher) {
		m_publisher = publisher;
		emit publisherChanged();
	}
}

void DownloadCardModel::setVideoSize(qint64 videoSize)
{
	if (m_videoSize != videoSize) {
		m_videoSize = videoSize;
		emit videoSizeChanged();
	}
}

void DownloadCardModel::setAudioSize(qint64 audioSize)
{
	if (m_audioSize != audioSize) {
		m_audioSize = audioSize;
		emit audioSizeChanged();
	}
}

void DownloadCardModel::setState(DownloadCardState state)
{
	if (m_state != state) {
		m_state = state;
		emit stateChanged();
	}
}

void DownloadCardModel::setProgress(int progress)
{
	if (m_progress != progress) {
		m_progress = progress;
		emit progressChanged();
	}
}

void DownloadCardModel::setDownloadSpeed(qint64 downloadSpeed)
{
	if (m_downloadSpeed != downloadSpeed) {
		m_downloadSpeed = downloadSpeed;
		emit downloadSpeedChanged();
	}
}

void DownloadCardModel::setFilePath(const QString& filePath)
{
	if (m_filePath != filePath) {
		m_filePath = filePath;
		emit filePathChanged();
	}
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
	if (m_audioQuality != quality) {
		m_audioQuality = quality;
		emit audioQualityChanged();
	}
}

void DownloadCardModel::setDownloadedSize(qint64 downloadedSize)
{
	if (m_downloadedSize != downloadedSize) {
		m_downloadedSize = downloadedSize;
		emit progressChanged();
	}
}

void DownloadCardModel::setDownloadSize(qint64 totalSize)
{
	if (m_downloadSize != totalSize) {
		m_downloadSize = totalSize;
		emit progressChanged();
	}
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
	switch (taskInfo.status) {
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

	// 触发所有信号
	emit taskIdChanged();
	emit titleChanged();
	emit stateChanged();
	emit progressChanged();
	emit downloadSpeedChanged();
}