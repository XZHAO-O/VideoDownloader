#include "BadgeWidget.h"
#include <QPainter>

BadgeWidget::BadgeWidget(int width, int height, QWidget* parent)
	: QWidget(parent), m_count(0), showCount(false), m_isCircle(true), m_maxCount(99)
{
	setFixedSize(width, height);  // 设置大小

	m_font.setBold(true);
	m_font.setPointSizeF(9.5);
}

BadgeWidget::~BadgeWidget()
{
}

void BadgeWidget::setBadge(bool isCircle, BadgeStatus status, int count, int maxCount)
{
	m_isCircle = isCircle;
	m_maxCount = maxCount;
	this->m_count = count;
	showCount = (count > 0);  // 如果 count 大于 0，则显示数字
	m_status = status;

	// 根据状态选择颜色

	switch (m_status)
	{
	case Success:
		m_badgeColor = QColor(76, 175, 80); // 绿色（成功）
		break;
	case Error:
		m_badgeColor = QColor(244, 67, 54); // 红色（错误）
		break;
	case Running:
		m_badgeColor = QColor(22, 119, 255); // （运行中）
		break;
	case Warning:
		m_badgeColor = QColor(255, 235, 59); // 黄色（警告）
		break;
	case Default:
		m_badgeColor = QColor(189, 189, 189); // 灰色（默认）
		break;
	case None:
		break;
	default:
		break;
	}

	update();
}

void BadgeWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 根据类型选择绘制圆形还是圆角矩形
	if (m_status == None)
	{
		if (m_isCircle)
		{
			// 绘制红色的小圆点
			painter.setBrush(QColor(255, 59, 48)); // 红色
			painter.setPen(Qt::NoPen);  // 无边框
			painter.drawEllipse(0, 0, width(), height());  // 绘制圆形
		}
		else
		{
			// 绘制红色的胶囊型圆角矩形
			painter.setBrush(QColor(255, 59, 48)); // 红色
			painter.setPen(Qt::NoPen);  // 无边框
			painter.drawRoundedRect(0, 0, width(), height(), height() / 2, height() / 2);
		}
	}
	else
	{
		// 绘制状态圆点
		painter.setBrush(m_badgeColor);
		painter.setPen(Qt::NoPen);  // 无边框
		painter.drawEllipse(0, 0, width(), height());  // 绘制圆形
	}

	// 如果需要显示数量
	if (showCount && m_count > 0)
	{
		QString countText;

		// 如果数量超过最大值，显示最大值并加上 "+"
		if (m_count > m_maxCount)
		{
			countText = QString::number(m_maxCount) + "+";  // 超过最大值时，显示最大值并加上 "+"
		}
		else
		{
			countText = QString::number(m_count);  // 否则显示当前数量
		}

		painter.setPen(QColor(255, 255, 255));  // 白色文字
		painter.setFont(m_font);  // 使用自定义字体
		painter.drawText(rect(), Qt::AlignCenter, countText);  // 显示数量
	}
}