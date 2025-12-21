#include "LoadingArc.h"

#include <QPainter>
#include <QTimer>
#include <QResizeEvent>
#include "DesignSystem.h"

LoadingArc::LoadingArc(QWidget* parent)
	: QWidget(parent)
	, m_arcColor(DesignSystem::instance()->primaryColor())
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
	setAutoFillBackground(false);
	setFixedSize(38, 38);

	m_timer = new QTimer(this);
	m_timer->setTimerType(Qt::PreciseTimer);
	m_timer->setInterval(50);  // 每帧间隔（毫秒），可调

	connect(m_timer, &QTimer::timeout, this, &LoadingArc::updateArc, Qt::DirectConnection);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
		m_arcColor = DesignSystem::instance()->primaryColor();
		m_needUpdatePen = true;
		update();
		});

	updateCachedObjects();
}

LoadingArc::~LoadingArc() {
	stop();
}

void LoadingArc::start() {
	if (m_timer && !m_timer->isActive()) {
		m_currentFrame = 0;
		m_timer->start();
	}
}

void LoadingArc::stop() {
	if (m_timer) {
		m_timer->stop();
	}
	m_currentFrame = 0;
	update();
}

void LoadingArc::setUpdateInterval(int ms) {
	if (m_timer) {
		m_timer->setInterval(ms);
	}
}

void LoadingArc::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	updateCachedObjects();
}

void LoadingArc::updateCachedObjects() {
	const int size = qMin(width(), height());
	m_cachedThickness = qMax(size / 8, 2);

	const qreal halfThickness = m_cachedThickness / 2.0;
	m_cachedRect = QRectF(
		halfThickness,
		halfThickness,
		size - m_cachedThickness,
		size - m_cachedThickness
	);

	m_needUpdatePen = true;
}

void LoadingArc::updateArc() {
	m_currentFrame = (m_currentFrame + 1) % m_totalFrames;
	repaint();
}

void LoadingArc::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);

	if (m_needUpdatePen) {
		m_cachedPen = QPen(m_arcColor);
		m_cachedPen.setWidth(m_cachedThickness);
		m_cachedPen.setCapStyle(Qt::RoundCap);
		m_needUpdatePen = false;
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(m_cachedPen);

	// 计算当前帧的起始角度
	int startAngle = (m_currentFrame * m_angleStep) * 16;
	int angleSpan = 120 * 16;  // 圆弧跨度，可调

	painter.drawArc(m_cachedRect, startAngle, angleSpan);
}