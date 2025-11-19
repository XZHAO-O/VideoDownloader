#pragma once

#include <QUrl>
#include <QDateTime>
#include <QByteArray>

class DownloadTaskInfo;

enum class DownloadCardState
{
	Pending,        // 待下载
	Downloading,    // 下载中
	Downloaded,     // 已下载
	Error           // 错误
};

class DownloadCardModel : public QObject
{
	Q_OBJECT

		Q_PROPERTY(QString taskId READ taskId WRITE setTaskId NOTIFY taskIdChanged)
		Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
		Q_PROPERTY(QUrl coverUrl READ coverUrl WRITE setCoverUrl NOTIFY coverUrlChanged)
		Q_PROPERTY(QByteArray cover READ cover WRITE setCover NOTIFY coverChanged)
		Q_PROPERTY(QString duration READ duration WRITE setDuration NOTIFY durationChanged)
		Q_PROPERTY(QString publishTime READ publishTime WRITE setPublishTime NOTIFY publishTimeChanged)
		Q_PROPERTY(QString publisher READ publisher WRITE setPublisher NOTIFY publisherChanged)
		Q_PROPERTY(qint64 videoSize READ videoSize WRITE setVideoSize NOTIFY videoSizeChanged)
		Q_PROPERTY(qint64 audioSize READ audioSize WRITE setAudioSize NOTIFY audioSizeChanged)
		Q_PROPERTY(DownloadCardState state READ state WRITE setState NOTIFY stateChanged)
		Q_PROPERTY(QString progressInfo READ progressInfo WRITE setProgressInfo NOTIFY progressInfoChanged)
		Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
		Q_PROPERTY(QString downloadSpeed READ downloadSpeed WRITE setDownloadSpeed NOTIFY downloadSpeedChanged)
		Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
		Q_PROPERTY(QString videoQuality READ videoQuality WRITE setVideoQuality NOTIFY videoQualityChanged)
		Q_PROPERTY(QString audioQuality READ audioQuality WRITE setAudioQuality NOTIFY audioQualityChanged)

public:
	explicit DownloadCardModel(QObject* parent = nullptr);
	explicit DownloadCardModel(QSharedPointer<DownloadTaskInfo> taskInfo, QObject* parent = nullptr);

	// Getters
	const QString& taskId() const { return m_taskId; }
	const QString& title() const { return m_title; }
	const QUrl& coverUrl() const { return m_coverUrl; }
	const QByteArray& cover() const { return m_cover; }
	const QString& duration() const { return m_duration; }
	const QString& publishTime() const { return m_publishTime; }
	const QString& publisher() const { return m_publisher; }
	const qint64& videoSize() const { return m_videoSize; }
	const qint64& audioSize() const { return m_audioSize; }
	const DownloadCardState& state() const { return m_state; }
	const QString& progressInfo() const { return m_progressInfo; }
	const int& progress() const { return m_progress; }
	const QString& downloadSpeed() const { return m_downloadSpeed; }
	const QString& filePath() const { return m_filePath; }
	const QString& videoQuality() const { return m_videoQuality; }
	const QString& audioQuality() const { return m_audioQuality; }

	// Setters
	void setTaskId(const QString& taskId);
	void setTitle(const QString& title);
	void setCoverUrl(const QUrl& coverUrl);
	void setCover(const QByteArray& cover);
	void setDuration(const QString& duration);
	void setPublishTime(const QString& publishTime);
	void setPublisher(const QString& publisher);
	void setVideoSize(qint64 videoSize);
	void setAudioSize(qint64 audioSize);
	void setState(DownloadCardState state);
	void setProgressInfo(const QString& progressInfo);
	void setProgress(int progress);
	void setDownloadSpeed(const QString& downloadSpeed);
	void setFilePath(const QString& filePath);
	void setVideoQuality(const QString& quality);
	void setAudioQuality(const QString& quality);

	// 从DownloadTaskInfo转换
	void fromDownloadTaskInfo(QSharedPointer<DownloadTaskInfo> taskInfo);

signals:
	void taskIdChanged();
	void titleChanged();
	void coverUrlChanged();
	void coverChanged();
	void durationChanged();
	void publishTimeChanged();
	void publisherChanged();
	void videoSizeChanged();
	void audioSizeChanged();
	void stateChanged();
	void progressInfoChanged();
	void progressChanged();
	void downloadSpeedChanged();
	void filePathChanged();
	void videoQualityChanged();
	void audioQualityChanged();

private:
	QString m_taskId;
	QString m_title;
	QUrl m_coverUrl;
	QByteArray m_cover;
	QString m_duration;
	QString m_publishTime;
	QString m_publisher;
	qint64 m_videoSize = 0;
	qint64 m_audioSize = 0;
	DownloadCardState m_state = DownloadCardState::Pending;
	QString m_progressInfo;
	int m_progress = 0;
	QString m_downloadSpeed = 0;
	QString m_filePath;
	QString m_videoQuality;
	QString m_audioQuality;
};