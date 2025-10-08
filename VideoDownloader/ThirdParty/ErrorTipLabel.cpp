#include "ErrorTipLabel.h"
#include <QPainter>

ErrorTipLabel::ErrorTipLabel(QWidget* parent)
	: QLabel(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	QFont font;
	font.setPointSizeF(10);
	setFont(font);

	m_opacityAnimation = new QPropertyAnimation(this, "opacity");
	m_opacityAnimation->setDuration(200);
}

ErrorTipLabel::~ErrorTipLabel()
{
}

void ErrorTipLabel::reposition()
{
	if (!m_targetWidget) return;

	// 获取目标控件的左下角全局坐标
	QPoint targetBottomLeft = m_targetWidget->mapToGlobal(QPoint(0, m_targetWidget->height()));

	// 转换为当前父窗口内坐标
	QPoint parentPos = parentWidget()->mapFromGlobal(targetBottomLeft);

	int x = parentPos.x();          // 左对齐
	int y = parentPos.y() + 4;      // 间距 4 像素下移

	move(x, y);
}

void ErrorTipLabel::setTargetWidget(QWidget* widget)
{
	m_targetWidget = widget;
	if (m_targetWidget) {
		reposition();
	}
}

void ErrorTipLabel::showError(const QString text)
{
	m_errorText = text;
	setText(text);
	show();        // 显示控件本身

	m_opacityAnimation->stop();
	m_opacityAnimation->setStartValue(0.0);
	m_opacityAnimation->setEndValue(1.0);
	m_opacityAnimation->start();
}

void ErrorTipLabel::hideError()
{
	m_opacityAnimation->stop();
	m_opacityAnimation->setStartValue(1.0);
	m_opacityAnimation->setEndValue(0.0);
	m_opacityAnimation->start();

	connect(m_opacityAnimation, &QPropertyAnimation::finished, this, [this]()
		{
			this->hide();
		});
}

void ErrorTipLabel::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 设置透明度
	painter.setOpacity(m_opacity);

	// 设置字体颜色为红色
	painter.setPen(QColor(255, 77, 79));

	// 绘制文本，保持与 QLabel 类似的布局
	painter.setFont(font());
	painter.drawText(rect(), alignment(), m_errorText);
}