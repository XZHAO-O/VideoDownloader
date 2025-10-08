#include "FramelessVideoWindow.h"
#include "DesignSystem.h"
#include <QFile>
#include <QProcess>

FramelessVideoWindow::FramelessVideoWindow(const QString& videoPath, QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	m_scene = new QGraphicsScene(this);
	m_videoItem = new QGraphicsVideoItem();
	m_videoItem->setAspectRatioMode(Qt::KeepAspectRatioByExpanding);
	m_scene->addItem(m_videoItem);

	m_view = new QGraphicsView(m_scene, this);
	// 判断是否使用 OpenGL 视口
	if (!isVirtualGPU())
	{
		m_view->setViewport(new QOpenGLWidget);
		qDebug() << "OpenGL viewport enabled";
	}
	else
	{
		qDebug() << "Virtual GPU detected, using default viewport";
	}
	m_view->setFrameShape(QFrame::NoFrame);
	m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_view->setStyleSheet("background-color: transparent;");
	m_view->viewport()->setAttribute(Qt::WA_TransparentForMouseEvents, true);

	m_player = new QMediaPlayer(this);
	m_player->setVideoOutput(m_videoItem);
	m_player->setSource(QUrl::fromLocalFile(videoPath));
	m_player->setLoops(QMediaPlayer::Infinite); // 无限循环

	// 模糊玻璃
	glassWidget1 = new GlassWidget(0, 0, this);
	glassWidget1->setPixmapAndText(":/Imgs/github.png", "QtAntDesign Github", "如果你觉得这个有用，欢迎在GitHub上点个Star支持一下~");
	glassWidget1->setUrl(QUrl("https://github.com/byralpha/AntDesign"));
	glassWidget2 = new GlassWidget(10, 3, this);
	glassWidget2->setPixmapAndText(":/Imgs/bee.png", "QtAntDesign Widget", "572205527@qq.com");

	// 防抖定时器
	m_blurTimer = new QTimer(this);
	m_blurTimer->setInterval(200);	// 模糊刷新时间
	connect(m_blurTimer, &QTimer::timeout, this, [=]()
		{
			glassWidget2->clearImage();

			// 获取 glassWidget 的几何区域
			QRect glassRect = glassWidget2->geometry();

			// 创建一个 QImage 对象，用来接收渲染的内容
			QImage image(glassRect.size(), QImage::Format_ARGB32_Premultiplied);

			// 使用 QPainter 将指定区域的内容渲染到 QImage
			QPainter painter(&image);
			m_scene->render(&painter, QRectF(0, 0, glassRect.width(), glassRect.height()), glassRect);

			// 设置为背景图
			glassWidget2->setBackgroundImage(image);
		});
}

FramelessVideoWindow::~FramelessVideoWindow()
{
}

void FramelessVideoWindow::resizeEvent(QResizeEvent* event)
{
	// 确保视图和视频大小和窗口一致
	m_scene->setSceneRect(rect());
	m_view->setFixedSize(size());
	m_videoItem->setSize(QSizeF(size()));
	glassWidget1->setFixedSize(width() * 1 / 6 + 60, height() * 1 / 5 + 60);
	glassWidget2->setFixedSize(width() * 1 / 6 + 60, height() * 1 / 5 + 60);
	glassWidget1->move(width() * 2 / 3, height() / 2 - 260);
	glassWidget2->move(width() * 2 / 3, height() / 2 - 260 + glassWidget1->height() + 30);
}

void FramelessVideoWindow::showEvent(QShowEvent* event)
{
	// 当窗口显示时，启动视频播放
	if (m_player->playbackState() != QMediaPlayer::PlayingState)
	{
		m_player->play();
		m_blurTimer->start();
	}
}

void FramelessVideoWindow::hideEvent(QHideEvent* event)
{
	// 当窗口隐藏时，视频播放
	if (m_player->playbackState() == QMediaPlayer::PlayingState)
	{
		m_player->stop();
		m_blurTimer->stop();
	}
}

bool FramelessVideoWindow::isVirtualGPU()
{
#ifdef Q_OS_LINUX
	QProcess proc;
	proc.start("lspci");
	proc.waitForFinished();
	QString output = proc.readAllStandardOutput();

	// 常见虚拟 GPU 标识
	if (output.contains("VMware") || output.contains("VirtualBox") || output.contains("QXL") || output.contains("SVGA"))
		return true;
	return false;
#else
	return false;
#endif
}