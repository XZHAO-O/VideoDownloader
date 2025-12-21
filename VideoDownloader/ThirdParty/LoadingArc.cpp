#include "LoadingArc.h"

#include <QPainter>
#include <QTimer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include "DesignSystem.h"

LoadingArc::LoadingArc(QWidget* parent)
	: QWidget(parent)
	, m_arcColor(DesignSystem::instance()->primaryColor())
{
	// 优化绘制性能的设置
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
	setAutoFillBackground(false);
	setFixedSize(38, 38);

	// 使用高效的定时器设置
	m_timer = new QTimer(this);
	m_timer->setTimerType(Qt::CoarseTimer);
	m_timer->setInterval(33); // 约30fps

	connect(m_timer, &QTimer::timeout, this, &LoadingArc::updateArc);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
		m_arcColor = DesignSystem::instance()->primaryColor();
		m_needUpdatePen = true;
		m_bufferDirty = true; // 颜色改变，标记缓冲区需要更新
		update();
		});

	updateCachedObjects();
}

LoadingArc::~LoadingArc() {
	stop();
}

void LoadingArc::start() {
	if (m_timer && !m_timer->isActive()) {
		m_currentAngle = 0.0;
		m_timer->start();
	}
}

void LoadingArc::stop() {
	if (m_timer) {
		m_timer->stop();
	}
	m_currentAngle = 0.0;
	update();
}

void LoadingArc::setUpdateInterval(int ms) {
	if (m_timer) {
		m_timer->setInterval(ms);
	}
}

void LoadingArc::setRotationSpeed(int degreesPerSecond) {
	m_rotationSpeed = degreesPerSecond;
}

void LoadingArc::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	if (!m_timer->isActive()) {
		start();
	}
}

void LoadingArc::hideEvent(QHideEvent* event) {
	QWidget::hideEvent(event);
	stop();
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
	m_bufferDirty = true; // 尺寸改变，标记缓冲区需要更新
}

void LoadingArc::updateBuffer() {
	// 创建缓冲区
	m_buffer = QPixmap(size());
	m_buffer.fill(Qt::transparent);

	QPainter bufferPainter(&m_buffer);
	bufferPainter.setRenderHint(QPainter::Antialiasing, true);

	// 更新画笔
	if (m_needUpdatePen) {
		m_cachedPen = QPen(m_arcColor);
		m_cachedPen.setWidth(m_cachedThickness);
		m_cachedPen.setCapStyle(Qt::RoundCap);
		m_needUpdatePen = false;
	}

	bufferPainter.setPen(m_cachedPen);

	// 绘制一个固定位置的圆弧（不旋转）
	// 我们绘制一个从-60度开始的120度圆弧，这样旋转时看起来像是从顶部开始
	int startAngle = -60 * 16; // 从-60度开始
	int angleSpan = 120 * 16;   // 120度圆弧

	bufferPainter.drawArc(m_cachedRect, startAngle, angleSpan);

	m_bufferDirty = false;
}

void LoadingArc::updateArc() {
	// 根据速度和时间间隔更新角度
	m_currentAngle += m_rotationSpeed * m_timer->interval() / 1000.0;

	// 归一化到0-360度范围内
	if (m_currentAngle >= 360.0) {
		m_currentAngle -= 360.0;
	}

	update();
}

void LoadingArc::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);

	// 如果缓冲区需要更新，重新生成
	if (m_bufferDirty) {
		updateBuffer();
	}

	// 如果缓冲区为空或尺寸不匹配，重新生成
	if (m_buffer.isNull() || m_buffer.size() != size()) {
		updateBuffer();
	}

	// 绘制旋转的缓冲区
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// 保存当前画家的状态
	painter.save();

	// 平移和旋转
	painter.translate(width() / 2.0, height() / 2.0);
	painter.rotate(m_currentAngle);
	painter.translate(-width() / 2.0, -height() / 2.0);

	// 绘制缓冲区
	painter.drawPixmap(0, 0, m_buffer);

	// 恢复画家的状态
	painter.restore();
}