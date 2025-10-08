#include "AnimatedNumber.h"
#include <QPainter>
#include <QFontMetrics>
#include "DesignSystem.h"

AnimatedNumber::AnimatedNumber(QWidget* parent)
	: QWidget(parent)
{
	m_animation = new QVariantAnimation(this);
	m_animation->setEasingCurve(QEasingCurve::InOutCubic);
	m_animation->setLoopCount(-1);

	connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
		setDisplayedValue(value.toInt());
		});
}

int AnimatedNumber::displayedValue() const
{
	return m_displayedValue;
}

void AnimatedNumber::setDisplayedValue(int val)
{
	if (m_displayedValue != val) {
		m_displayedValue = val;
		update();
	}
}

void AnimatedNumber::animateTo(int target)
{
	int diff = qAbs(target - m_displayedValue);
	int maxDuration = 10000;   // 最大10秒
	int minDuration = 200;     // 最短200ms

	// 根据数值差调整时长，差越大，时间越长，线性映射
	int duration = qBound(minDuration, diff * 20, maxDuration);
	// diff*20是个系数，可以调节动画快慢

	m_animation->stop();
	m_animation->setDuration(duration);
	m_animation->setStartValue(m_displayedValue);
	m_animation->setEndValue(target);
	m_animation->start();
}

void AnimatedNumber::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	QFont font = p.font();
	font.setPointSize(m_fontSize);
	font.setBold(true);
	p.setFont(font);
	p.setPen(DesignSystem::instance()->currentTheme().msgTextColor);

	QString text = QString::number(m_displayedValue);
	QFontMetrics fm(font);

	// 计算文本的边界矩形
	QRect textRect = fm.boundingRect(text);

	// 计算居中位置
	QRect drawRect(0, 0, width(), height());
	p.drawText(drawRect, Qt::AlignCenter, text);
}

QSize AnimatedNumber::sizeHint() const
{
	QFont font;
	font.setPointSize(m_fontSize);
	font.setBold(true);
	QFontMetrics fm(font);
	int h = fm.height() + 10; // 额外留点空间
	int w = m_textWidth;              // 固定宽度，或者算一个合适值
	return QSize(w, h);
}