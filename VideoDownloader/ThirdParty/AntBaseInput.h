#ifndef ANTBASEINPUT_H
#define ANTBASEINPUT_H

#include <QLineEdit>
#include <QToolButton>
#include <QColor>

#include <QStyle>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>

class AntBaseInputStyle : public QProxyStyle
{
public:
	AntBaseInputStyle(QColor normalBg, QColor hoverBg, QColor focusBorderColor, QColor focusBg, QColor shadowColor, int borderWidth)
		: m_normalBg(normalBg),
		m_hoverBg(hoverBg),
		m_focusBorderColor(focusBorderColor),
		m_focusBg(focusBg),
		m_borderWidth(borderWidth),
		m_shadowColor(shadowColor)
	{
	}

	QRect subElementRect(SubElement r, const QStyleOption* option, const QWidget* widget) const override
	{
		QRect rect = QProxyStyle::subElementRect(r, option, widget);
		if (widget && widget->objectName() == "AntBaseInput" && r == SE_LineEditContents) {
			int padding = 6;
			rect.adjust(padding, 0, -padding, 0);
		}
		return rect;
	}

	void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override
	{
		if (element == PE_PanelLineEdit && widget && widget->objectName() == "AntBaseInput") {
			painter->save();
			painter->setRenderHint(QPainter::Antialiasing);

			QColor bgColor = m_normalBg;
			QColor borderColor = Qt::transparent;

			int spread = 6;
			QRect rect = option->rect.adjusted(spread, spread,
				-spread, -spread);

			if (option->state & State_HasFocus) {
				bgColor = m_focusBg;
				borderColor = m_focusBorderColor;

				int baseAlpha = 80;
				int radius = 8;

				for (int i = 0; i < spread; ++i)
				{
					int alpha = baseAlpha * (1.0f - static_cast<float>(i) / spread);
					QColor shadow = m_shadowColor;
					shadow.setAlpha(alpha);
					QPen pen(shadow, 1.2);
					painter->setPen(pen);
					painter->setBrush(Qt::NoBrush);
					QRect shadowRect = rect.adjusted(-i, -i, i, i);
					painter->drawRoundedRect(shadowRect, radius + i, radius + i);
				}
			}
			else if (option->state & State_MouseOver) {
				bgColor = m_hoverBg;
			}

			// 画背景
			painter->setBrush(bgColor);
			painter->setPen(Qt::NoPen);
			painter->drawRoundedRect(rect, 8, 8);

			// 画边框
			if (borderColor.alpha() > 0) {
				QPen pen(borderColor, m_borderWidth);
				painter->setPen(pen);
				painter->setBrush(Qt::NoBrush);
				painter->drawRoundedRect(rect, 8, 8);
			}

			painter->restore();

			// 不继续默认绘制背景
			return;
		}

		// 其他情况默认绘制
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}

	void updateStyle(QColor normalBg, QColor hoverBg, QColor focusBorderColor, QColor focusBg, QColor shadowColor)
	{
		m_normalBg = normalBg;
		m_hoverBg = hoverBg;
		m_focusBorderColor = focusBorderColor;
		m_focusBg = focusBg;
		m_shadowColor = shadowColor;
	}
private:
	QColor m_normalBg;
	QColor m_hoverBg;
	QColor m_focusBorderColor;
	QColor m_focusBg;
	QColor m_shadowColor;
	int m_borderWidth;
};

class AntBaseInput : public QLineEdit
{
	Q_OBJECT

public:
	explicit AntBaseInput(QWidget* parent = nullptr);
private:
	AntBaseInputStyle* style;
	QColor m_backgroundColor;		// 背景色
	QColor m_hoverBackgroundColor;  // 鼠标悬停时背景色
	QColor m_focusBackgroundColor;  // 聚焦时背景色
	QColor m_focusBorderColor;		// 聚焦时边框颜色
};

#endif // ANTBASEINPUT_H
