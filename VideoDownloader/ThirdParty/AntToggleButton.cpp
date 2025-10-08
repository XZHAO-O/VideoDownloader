#include "AntToggleButton.h"
#include <QPainter>
#include <QMouseEvent>
#include "DesignSystem.h"

AntToggleButton::AntToggleButton(QSize size, QWidget* parent)
	: QWidget(parent), m_checked(false), m_size(size), m_bgColor(DesignSystem::instance()->currentTheme().toggleButtonBgColor),
	m_toggleButtonColor(DesignSystem::instance()->currentTheme().toggleButtonColor), m_textColor(DesignSystem::instance()->currentTheme().textColor)
{
	setFixedSize(m_size);

	m_circleWidth = height() - 6;
	m_circleX = 3;

	// 滑块位置动画
	posAnim = new QPropertyAnimation(this, "m_circleX", this);
	posAnim->setDuration(200);
	posAnim->setEasingCurve(QEasingCurve::InOutQuad);

	// 背景颜色动画
	colorAnim = new QPropertyAnimation(this, "m_bgColor", this);
	colorAnim->setDuration(200);
	colorAnim->setEasingCurve(QEasingCurve::InOutQuad);

	// 动画组
	groupAnim = new QParallelAnimationGroup(this);
	groupAnim->addAnimation(posAnim);
	groupAnim->addAnimation(colorAnim);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_bgColor = DesignSystem::instance()->currentTheme().toggleButtonBgColor;
			m_toggleButtonColor = DesignSystem::instance()->currentTheme().toggleButtonColor;
			m_textColor = DesignSystem::instance()->currentTheme().textColor;
			update();
		});
}

AntToggleButton::~AntToggleButton() {}

void AntToggleButton::setShowText(bool show)
{
	if (m_showText != show)
	{
		m_showText = show;
		update(); // 重新绘制，显示或隐藏文字
	}
}

void AntToggleButton::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	p.setPen(Qt::NoPen);
	p.setBrush(m_bgColor);
	p.drawRoundedRect(rect(), height() / 2, height() / 2);

	// 绘制滑块
	p.setBrush(m_toggleButtonColor);
	p.drawEllipse(m_circleX, 3, m_circleWidth, height() - 6);

	// 如果需要显示文字
	if (m_showText)
	{
		p.setPen(m_textColor);
		QFont font = p.font();
		font.setPointSizeF(9.4);
		font.setBold(true);
		p.setFont(font);

		QString text = m_checked ? QStringLiteral("ON") : QStringLiteral("OFF");
		// 文字位置：滑块中心附近，稍微偏右或偏左点，具体可调整
		QFontMetrics fm(font);
		int textWidth = fm.horizontalAdvance(text);
		int textHeight = fm.height();

		int xText;
		if (m_checked)
		{
			// 滑块在右边，文字画在滑块左边一些
			xText = m_circleX - textWidth - 2;
		}
		else
		{
			// 滑块在左边，文字画在滑块右边一点
			xText = m_circleX + m_circleWidth + 1;
		}
		int yText = (height() + textHeight) / 2 - fm.descent();

		p.drawText(xText, yText, text);
	}
}

void AntToggleButton::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && groupAnim->state() != QAbstractAnimation::Running)
	{
		setChecked(!m_checked);
	}

	QWidget::mousePressEvent(event);
}

void AntToggleButton::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_circleWidth = height() - 6;
	m_circleX = m_checked ? width() - height() + 3 : 3;
}

void AntToggleButton::setChecked(bool checked)
{
	if (m_checked != checked) {
		m_checked = checked;

		posAnim->stop();
		posAnim->setStartValue(m_circleX);
		posAnim->setEndValue(m_checked ? width() - height() + 3 : 3);

		colorAnim->stop();
		colorAnim->setStartValue(m_bgColor);
		colorAnim->setEndValue(m_checked ? DesignSystem::instance()->primaryColor()
			: DesignSystem::instance()->currentTheme().noCheckedColor);

		groupAnim->start();

		emit toggled(m_checked);
	}
}