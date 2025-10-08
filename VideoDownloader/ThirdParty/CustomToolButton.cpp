#include "CustomToolButton.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QMouseEvent>
#include "DesignSystem.h"

CustomToolButton::CustomToolButton(QSize iconSize, QWidget* parent)
	: QToolButton(parent), hovered(false), m_iconSize(iconSize)
{
	setCheckable(false);     // 非 toggle 状态
	setCursor(Qt::PointingHandCursor);
	setAttribute(Qt::WA_Hover); // 开启 hover 事件
	setStyleSheet("border: none; background: transparent;"); // 去除边框
}

CustomToolButton::~CustomToolButton()
{
	if (m_normalRenderer) delete m_normalRenderer;
	if (m_activeRenderer) delete m_activeRenderer;
}

void CustomToolButton::setSvgIcons(const QString& normalPath, const QString& activePath)
{
	if (m_normalRenderer) delete m_normalRenderer;
	if (m_activeRenderer) delete m_activeRenderer;

	m_normalRenderer = new QSvgRenderer(normalPath, this);
	m_activeRenderer = new QSvgRenderer(activePath, this);
	update();
}

void CustomToolButton::enterEvent(QEnterEvent* event)
{
	if (!isEnabled()) return;
	hovered = true;
	update();
	QToolButton::enterEvent(event);
}

void CustomToolButton::leaveEvent(QEvent* event)
{
	hovered = false;
	update();
	QToolButton::leaveEvent(event);
}

void CustomToolButton::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_checked = true;
		update(); // 刷新图标
	}

	QToolButton::mousePressEvent(event); // 保留原事件逻辑
}

void CustomToolButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing);

	// 只有 hovered 或 checked 时才绘制背景
	if (hovered || m_checked)
	{
		// 背景颜色更亮一些
		QColor bgColor = DesignSystem::instance()->currentTheme().toolBtnBgColor;

		// 缩小绘制圆角背景
		QRectF rect = this->rect().adjusted(7, 7, -7, -7);
		QPainterPath path;
		path.addRoundedRect(rect, 8, 8);
		painter.fillPath(path, bgColor);
	}

	// 根据 isChecked 选择 SVG 渲染器
	QSvgRenderer* currentRenderer = m_checked ? m_activeRenderer : m_normalRenderer;

	if (currentRenderer)
	{
		QPoint iconTopLeft((width() - m_iconSize.width()) / 2, (height() - m_iconSize.height()) / 2);
		QRect iconRect(iconTopLeft, m_iconSize);
		currentRenderer->render(&painter, iconRect);
	}
}