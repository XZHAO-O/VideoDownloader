// MaterialTabBar.cpp
#include "MaterialTabBar.h"
#include <QPainter>
#include "StyleSheet.h"
#include <QTimer>
#include "DesignSystem.h"

MaterialTabBar::MaterialTabBar(QWidget* parent)
	:QTabBar(parent), m_indicatorPosX(0), m_indicatorWidth(0)
{
	auto theme = DesignSystem::instance()->currentTheme();
	setStyleSheet(StyleSheet::hTabQss(theme.primaryColor, theme.tabTextColor));
	setDrawBase(false);
	setExpanding(false);
	QFont font;
	font.setPointSizeF(11.5);
	setFont(font);
	m_animation = new QPropertyAnimation(this, "indicatorPos", this);
	m_animation->setDuration(250);
	m_animation->setEasingCurve(QEasingCurve::OutCubic);

	connect(this, &QTabBar::currentChanged, this, [this, font](int index)
		{
			QRect rect = tabRect(index);
			QString text = tabText(index);
			QFontMetrics fm(font);
			int textWidth = fm.horizontalAdvance(text);
			m_indicatorWidth = textWidth;

			int targetX = rect.x() + (rect.width() - textWidth) / 2;

			// 如果只有一个标签，直接设置位置，不用动画
			if (count() == 1)
			{
				m_indicatorPosX = targetX;
				update();
				return;
			}

			// 多个标签时才使用动画
			m_animation->stop();
			m_animation->setStartValue(m_indicatorPosX);
			m_animation->setEndValue(targetX);
			m_animation->start();
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			auto theme = DesignSystem::instance()->currentTheme();
			setStyleSheet(StyleSheet::hTabQss(theme.primaryColor, theme.tabTextColor));
		});
}

MaterialTabBar::~MaterialTabBar()
{
}

void MaterialTabBar::setIndicatorPos(int pos)
{
	m_indicatorPosX = pos;
	update();
}

void MaterialTabBar::paintEvent(QPaintEvent* event)
{
	QTabBar::paintEvent(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	painter.setBrush(DesignSystem::instance()->primaryColor());
	QRect rect(m_indicatorPosX, height() - 3, m_indicatorWidth, 3);
	painter.drawRect(rect);
}

QSize MaterialTabBar::tabSizeHint(int index) const
{
	QString text = tabText(index);
	QFontMetrics fm(font());
	int textWidth = fm.horizontalAdvance(text);
	int padding = 40;
	return QSize(textWidth + padding, 60);
}

void MaterialTabBar::updateIndicatorPosition(int index)
{
	QRect rect = tabRect(index);
	QFontMetrics fm(font());
	QString text = tabText(index);
	int textWidth = fm.horizontalAdvance(text);
	m_indicatorWidth = textWidth;
	m_indicatorPosX = rect.x() + (rect.width() - textWidth) / 2;
}

void MaterialTabBar::resizeEvent(QResizeEvent* event)
{
	QTabBar::resizeEvent(event);

	if (count() > 0)
	{
		updateIndicatorPosition(currentIndex());
		update();
	}
}

void MaterialTabBar::wheelEvent(QWheelEvent* event)
{
	// 只在标签栏上处理滚轮事件
	if (rect().contains(mapFromGlobal(QCursor::pos()))) {
		int delta = event->angleDelta().y();
		if (delta > 0) {
			// 向上滚动，切换到前一个标签
			int newIndex = currentIndex() - 1;
			if (newIndex >= 0) {
				setCurrentIndex(newIndex);
			}
		}
		else if (delta < 0) {
			// 向下滚动，切换到后一个标签
			int newIndex = currentIndex() + 1;
			if (newIndex < count()) {
				setCurrentIndex(newIndex);
			}
		}
		event->accept();
	}
	else {
		// 鼠标不在标签栏上，忽略事件
		event->ignore();
	}
}