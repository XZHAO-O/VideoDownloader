#pragma once

#include <QUrl>
#include <QDateTime>

#include "DownloadCardState.h"

class DownloadTaskInfo;

class DownloadCardModel : public QObject
{
	Q_OBJECT

		Q_PROPERTY(QString taskId READ taskId WRITE setTaskId NOTIFY taskIdChanged)
		Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
		Q_PROPERTY(QUrl coverUrl READ coverUrl WRITE setCoverUrl NOTIFY coverUrlChanged)
		Q_PROPERTY(QString duration READ duration WRITE setDuration NOTIFY durationChanged)
		Q_PROPERTY(QDateTime publishTime READ publishTime WRITE setPublishTime NOTIFY publishTimeChanged)
		Q_PROPERTY(QString publisher READ publisher WRITE setPublisher NOTIFY publisherChanged)
		Q_PROPERTY(qint64 videoSize READ videoSize WRITE setVideoSize NOTIFY videoSizeChanged)
		Q_PROPERTY(qint64 audioSize READ audioSize WRITE setAudioSize NOTIFY audioSizeChanged)
		Q_PROPERTY(DownloadCardState state READ state WRITE setState NOTIFY stateChanged)
		Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
		Q_PROPERTY(qint64 downloadSpeed READ downloadSpeed WRITE setDownloadSpeed NOTIFY downloadSpeedChanged)
		Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
		Q_PROPERTY(VideoQualityLevel videoQuality READ videoQuality WRITE setVideoQuality NOTIFY videoQualityChanged)
		Q_PROPERTY(AudioQualityLevel audioQuality READ audioQuality WRITE setAudioQuality NOTIFY audioQualityChanged)
		Q_PROPERTY(qint64 downloadedSize READ downloadedSize WRITE setDownloadedSize NOTIFY downloadedSizeChanged)
		Q_PROPERTY(qint64 downloadSize READ downloadSize WRITE setDownloadSize NOTIFY downloadSizeChanged)

public:
	explicit DownloadCardModel(QObject* parent = nullptr);
	explicit DownloadCardModel(const DownloadTaskInfo& taskInfo, QObject* parent = nullptr);

	// Getters
	QString taskId() const { return m_taskId; }
	QString title() const { return m_title; }
	QUrl coverUrl() const { return m_coverUrl; }
	QString duration() const { return m_duration; }
	QDateTime publishTime() const { return m_publishTime; }
	QString publisher() const { return m_publisher; }
	qint64 videoSize() const { return m_videoSize; }
	qint64 audioSize() const { return m_audioSize; }
	DownloadCardState state() const { return m_state; }
	int progress() const { return m_progress; }
	qint64 downloadSpeed() const { return m_downloadSpeed; }
	QString filePath() const { return m_filePath; }
	VideoQualityLevel videoQuality() const { return m_videoQuality; }
	AudioQualityLevel audioQuality() const { return m_audioQuality; }
	qint64 downloadedSize() const { return m_downloadedSize; }
	qint64 downloadSize() const { return m_downloadSize; }

	// Setters
	void setTaskId(const QString& taskId);
	void setTitle(const QString& title);
	void setCoverUrl(const QUrl& coverUrl);
	void setDuration(const QString& duration);
	void setPublishTime(const QDateTime& publishTime);
	void setPublisher(const QString& publisher);
	void setVideoSize(qint64 videoSize);
	void setAudioSize(qint64 audioSize);
	void setState(DownloadCardState state);
	void setProgress(int progress);
	void setDownloadSpeed(qint64 downloadSpeed);
	void setFilePath(const QString& filePath);
	void setVideoQuality(VideoQualityLevel quality);
	void setAudioQuality(AudioQualityLevel quality);
	void setDownloadedSize(qint64 downloadedSize);
	void setDownloadSize(qint64 totalSize);
	// 工具方法
	QString formattedVideoSize() const;
	QString formattedAudioSize() const;
	QString formattedDownloadSpeed() const;
	QString formattedPublishTime() const;
	QString formattedDuration() const;

	// 从DownloadTaskInfo转换
	void fromDownloadTaskInfo(const DownloadTaskInfo& taskInfo);

signals:
	void taskIdChanged();
	void titleChanged();
	void coverUrlChanged();
	void durationChanged();
	void publishTimeChanged();
	void publisherChanged();
	void videoSizeChanged();
	void audioSizeChanged();
	void stateChanged();
	void progressChanged();
	void downloadSpeedChanged();
	void filePathChanged();
	void videoQualityChanged();
	void audioQualityChanged();
	void downloadedSizeChanged();
	void downloadSizeChanged();

private:
	QString m_taskId;
	QString m_title;
	QUrl m_coverUrl;
	QString m_duration;
	QDateTime m_publishTime;
	QString m_publisher;
	qint64 m_videoSize = 0;
	qint64 m_audioSize = 0;
	qint64 m_downloadSize;
	qint64 m_downloadedSize;
	DownloadCardState m_state = DownloadCardState::Pending;
	int m_progress = 0;
	qint64 m_downloadSpeed = 0;
	QString m_filePath;
	VideoQualityLevel m_videoQuality = VideoQualityLevel::High;
	AudioQualityLevel m_audioQuality = AudioQualityLevel::High;
};