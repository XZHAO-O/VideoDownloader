#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>

class QSlider;
class QPushButton;
class QLabel;

class VideoPreviewWindow : public QWidget
{
	Q_OBJECT

public:
	explicit VideoPreviewWindow(QWidget* parent = nullptr);
	~VideoPreviewWindow();

	void setVideoFile(const QString& filePath);
	void setVideoUrl(const QUrl& videoUrl);

protected:
	void closeEvent(QCloseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private slots:
	void onPlayPauseClicked();
	void onVolumeChanged(int volume);
	void onSeekPositionChanged(int position);
	void onDurationChanged(qint64 duration);
	void onPositionChanged(qint64 position);
	void onFullscreenClicked();
	void updateDurationInfo(qint64 currentInfo);

private:
	void initUI();
	void initConnections();

	QMediaPlayer* m_mediaPlayer = nullptr;
	QVideoWidget* m_videoWidget = nullptr;

	QSlider* m_positionSlider = nullptr;
	QSlider* m_volumeSlider = nullptr;
	QPushButton* m_playButton = nullptr;
	QPushButton* m_fullscreenButton = nullptr;
	QLabel* m_timeLabel = nullptr;

	QString m_currentFile;
	bool m_isFullscreen = false;
};