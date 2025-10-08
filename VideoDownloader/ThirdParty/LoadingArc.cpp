#include "LoadingArc.h"
#include <QPainter>
#include "DesignSystem.h"

LoadingArc::LoadingArc(QWidget* parent)
	: QWidget(parent), arcColor(DesignSystem::instance()->primaryColor())
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);

	m_animation = new QPropertyAnimation(this, "rotationAngle");
	m_animation->setStartValue(0);
	m_animation->setEndValue(360);
	m_animation->setDuration(1500); // 1.5秒一圈
	m_animation->setLoopCount(-1);  // 无限循环

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			arcColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

void LoadingArc::start() {
	if (m_animation && m_animation->state() != QAbstractAnimation::Running) {
		m_animation->start();
	}
}

void LoadingArc::stop() {
	if (m_animation) {
		m_animation->stop();
		m_rotationAngle = 0;
		update();
	}
}

void LoadingArc::setRotationAngle(qreal angle) {
	m_rotationAngle = angle;
	update();
}

void LoadingArc::paintEvent(QPaintEvent*) {
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	int w = width();
	int h = height();
	int size = qMin(w, h);
	int thickness = size / 8;

	QRectF rect((w - size) / 2 + thickness / 2, (h - size) / 2 + thickness / 2,
		size - thickness, size - thickness);

	p.translate(rect.center());
	p.rotate(m_rotationAngle);
	p.translate(-rect.center());

	QPen pen(arcColor);
	pen.setWidth(thickness);
	pen.setCapStyle(Qt::RoundCap);
	p.setPen(pen);

	// 画一个固定角度的圆弧（比如90度）
	p.drawArc(rect, 0, -90 * 16);  // Qt arc 单位是1/16°
}