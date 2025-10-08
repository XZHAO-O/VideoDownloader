#include "SkeletonWidget.h"
#include <QPainter>
#include <QLinearGradient>
#include "DesignSystem.h"

SkeletonWidget::SkeletonWidget(int rectRadius, QWidget* parent)
	: QWidget(parent), m_rectRadius(rectRadius)
{
	int barWidth = 140;
	m_animation = new QPropertyAnimation(this, "gradientX");
	m_animation->setDuration(1000);
	m_animation->setStartValue(-barWidth);
	m_animation->setEndValue(width() + barWidth);
	m_animation->setLoopCount(-1);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			update();
		});
}

void SkeletonWidget::setGradientX(int x)
{
	m_gradientX = x;
	update();
}

void SkeletonWidget::stopSkeleton()
{
	if (m_animation)
	{
		m_animation->stop();
	}
	this->hide();  // 隐藏骨架控件
}

void SkeletonWidget::startSkeleton()
{
	if (m_animation)
	{
		m_animation->stop();
		m_animation->start();
	}
	this->show();  // 隐藏骨架控件
}

void SkeletonWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// 背景色
	QRectF rect = this->rect();
	p.setBrush(DesignSystem::instance()->currentTheme().skeletonBgColor);
	p.setPen(Qt::NoPen);
	p.drawRoundedRect(rect, m_rectRadius, m_rectRadius);

	// 根据主题模式选择渐变色
	QColor highlightStartColor, highlightMidColor, highlightEndColor;

	if (DesignSystem::instance()->themeMode() == DesignSystem::ThemeMode::Light)
	{
		// 浅色模式：渐变颜色
		highlightStartColor = QColor(240, 240, 240, 0);		// 浅灰色，接近背景色
		highlightMidColor = QColor(255, 255, 255, 150);		// 白色，中间亮点
		highlightEndColor = QColor(240, 240, 240, 0);		// 浅灰色，渐变结束
	}
	else
	{
		// 深色模式：渐变颜色
		highlightStartColor = QColor(50, 50, 50, 0);		// 深灰色，接近背景色
		highlightMidColor = QColor(180, 180, 180, 150);		// 中等亮度灰色
		highlightEndColor = QColor(50, 50, 50, 0);			// 深灰色，渐变结束
	}

	// 创建渐变效果
	QLinearGradient gradient(m_gradientX - 70, 0, m_gradientX + 70, 0);
	gradient.setColorAt(0.0, highlightStartColor);   // 渐变起始颜色
	gradient.setColorAt(0.5, highlightMidColor);     // 渐变中间颜色
	gradient.setColorAt(1.0, highlightEndColor);     // 渐变结束颜色

	p.setBrush(gradient);
	p.drawRoundedRect(rect, m_rectRadius, m_rectRadius);
}

void SkeletonWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (m_animation) {
		int barWidth = 140;
		m_animation->setEndValue(width() + barWidth);
	}
}