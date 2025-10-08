#include "NoDataWidget.h"
#include <QPainter>
#include <QFontMetrics>
#include "DesignSystem.h"

NoDataWidget::NoDataWidget(QWidget* parent)
	: QWidget(parent)
{
	// 预加载 svg
	m_svgRenderer.load(m_svgPath);

	m_bgColor = DesignSystem::instance()->backgroundColor();

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_bgColor = DesignSystem::instance()->backgroundColor();
			update();
		});
}

void NoDataWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 背景填白，可修改为透明或其他色
	painter.fillRect(rect(), m_bgColor);

	// 根据控件大小计算svg绘制区域
	int baseSize = qMin(width(), height());

	int svgWidth = int(baseSize * m_factor);
	int svgHeight = int(svgWidth * m_scale);

	QRect svgRect(
		(width() - svgWidth) / 2,
		(height() - svgHeight) / 2 - svgHeight / 3,
		svgWidth, svgHeight);

	m_svgRenderer.render(&painter, svgRect);

	// 绘制文字
	QFont font = painter.font();
	font.setPointSize(14);
	painter.setFont(font);
	painter.setPen(QColor(150, 150, 150));

	QFontMetrics fm(font);
	int textWidth = fm.horizontalAdvance(m_text);
	int textHeight = fm.height();

	int textX = (width() - textWidth) / 2;
	int textY = svgRect.bottom() + 10 + textHeight;

	painter.drawText(textX, textY, m_text);
}