#include "VideoPreviewWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QAudioOutput>

VideoPreviewWindow::VideoPreviewWindow(QWidget* parent)
	: QWidget(parent)
{
	setWindowTitle("视频预览");
	setFixedSize(800, 600);
	setAttribute(Qt::WA_DeleteOnClose);

	initUI();
	initConnections();
}

VideoPreviewWindow::~VideoPreviewWindow()
{
	if (m_mediaPlayer) {
		m_mediaPlayer->stop();
	}
}

void VideoPreviewWindow::setVideoFile(const QString& filePath)
{
	m_currentFile = filePath;

	if (m_mediaPlayer && QFileInfo::exists(filePath)) {
		m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
		setWindowTitle(QString("视频预览 - %1").arg(QFileInfo(filePath).fileName()));
	}
}

void VideoPreviewWindow::setVideoUrl(const QUrl& videoUrl)
{
	if (m_mediaPlayer) {
		m_mediaPlayer->setSource(videoUrl);
		setWindowTitle("视频预览 - 网络视频");
	}
}

void VideoPreviewWindow::closeEvent(QCloseEvent* event)
{
	if (m_mediaPlayer) {
		m_mediaPlayer->stop();
	}
	event->accept();
}

void VideoPreviewWindow::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape && m_isFullscreen) {
		showNormal();
		m_isFullscreen = false;
	}
	else if (event->key() == Qt::Key_Space) {
		onPlayPauseClicked();
	}

	QWidget::keyPressEvent(event);
}

void VideoPreviewWindow::onPlayPauseClicked()
{
	if (!m_mediaPlayer) return;

	if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
		m_mediaPlayer->pause();
		m_playButton->setText("播放");
	}
	else {
		m_mediaPlayer->play();
		m_playButton->setText("暂停");
	}
}

void VideoPreviewWindow::onVolumeChanged(int volume)
{
	if (m_mediaPlayer) {
		m_mediaPlayer->setAudioOutput(new QAudioOutput);
		m_mediaPlayer->audioOutput()->setVolume(volume / 100.0);
	}
}

void VideoPreviewWindow::onSeekPositionChanged(int position)
{
	if (m_mediaPlayer && !m_positionSlider->isSliderDown()) {
		m_mediaPlayer->setPosition(position);
	}
}

void VideoPreviewWindow::onDurationChanged(qint64 duration)
{
	m_positionSlider->setRange(0, duration);
}

void VideoPreviewWindow::onPositionChanged(qint64 position)
{
	if (!m_positionSlider->isSliderDown()) {
		m_positionSlider->setValue(position);
	}
	updateDurationInfo(position);
}

void VideoPreviewWindow::onFullscreenClicked()
{
	if (m_isFullscreen) {
		showNormal();
		m_isFullscreen = false;
	}
	else {
		showFullScreen();
		m_isFullscreen = true;
	}
}

void VideoPreviewWindow::updateDurationInfo(qint64 currentInfo)
{
	QString tStr;
	if (currentInfo || m_mediaPlayer->duration()) {
		qint64 currentSeconds = currentInfo / 1000;
		qint64 totalSeconds = m_mediaPlayer->duration() / 1000;

		QTime currentTime((currentSeconds / 3600) % 60, (currentSeconds / 60) % 60, currentSeconds % 60, (currentInfo / 1000) % 1000);
		QTime totalTime((totalSeconds / 3600) % 60, (totalSeconds / 60) % 60, totalSeconds % 60, (m_mediaPlayer->duration() / 1000) % 1000);

		QString format = "mm:ss";
		if (totalSeconds > 3600)
			format = "hh:mm:ss";
		tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
	}
	m_timeLabel->setText(tStr);
}

void VideoPreviewWindow::initUI()
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(0, 0, 0, 0);

	// 视频显示区域
	m_videoWidget = new QVideoWidget(this);
	m_videoWidget->setStyleSheet("background-color: black;");

	// 控制面板
	QWidget* controlWidget = new QWidget(this);
	controlWidget->setFixedHeight(60);
	controlWidget->setStyleSheet("background-color: #2C2C2C;");

	QHBoxLayout* controlLayout = new QHBoxLayout(controlWidget);
	controlLayout->setSpacing(10);
	controlLayout->setContentsMargins(15, 8, 15, 8);

	// 播放/暂停按钮
	m_playButton = new QPushButton("播放", this);
	m_playButton->setFixedSize(60, 30);

	// 进度条
	m_positionSlider = new QSlider(Qt::Horizontal, this);
	m_positionSlider->setRange(0, 0);

	// 时间标签
	m_timeLabel = new QLabel("00:00 / 00:00", this);
	m_timeLabel->setStyleSheet("color: white; font-size: 12px;");
	m_timeLabel->setFixedWidth(100);

	// 音量滑块
	m_volumeSlider = new QSlider(Qt::Horizontal, this);
	m_volumeSlider->setRange(0, 100);
	m_volumeSlider->setValue(50);
	m_volumeSlider->setFixedWidth(80);

	// 全屏按钮
	m_fullscreenButton = new QPushButton("全屏", this);
	m_fullscreenButton->setFixedSize(60, 30);

	controlLayout->addWidget(m_playButton);
	controlLayout->addWidget(m_positionSlider);
	controlLayout->addWidget(m_timeLabel);
	controlLayout->addWidget(new QLabel("音量:", this));
	controlLayout->addWidget(m_volumeSlider);
	controlLayout->addWidget(m_fullscreenButton);

	mainLayout->addWidget(m_videoWidget, 1);
	mainLayout->addWidget(controlWidget);

	// 初始化媒体播放器
	m_mediaPlayer = new QMediaPlayer(this);
	m_mediaPlayer->setVideoOutput(m_videoWidget);
}

void VideoPreviewWindow::initConnections()
{
	connect(m_playButton, &QPushButton::clicked, this, &VideoPreviewWindow::onPlayPauseClicked);
	connect(m_volumeSlider, &QSlider::valueChanged, this, &VideoPreviewWindow::onVolumeChanged);
	connect(m_positionSlider, &QSlider::sliderMoved, this, &VideoPreviewWindow::onSeekPositionChanged);
	connect(m_fullscreenButton, &QPushButton::clicked, this, &VideoPreviewWindow::onFullscreenClicked);

	if (m_mediaPlayer) {
		connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &VideoPreviewWindow::onDurationChanged);
		connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &VideoPreviewWindow::onPositionChanged);
	}
}