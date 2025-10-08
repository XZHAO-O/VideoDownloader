#pragma once

#include <QWidget>
#include <QListView>
#include <QPropertyAnimation>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QProxyStyle>
#include <QPainterPath>
#include "DesignSystem.h"

// 自定义 Item Delegate 来控制 ListItem 的大小
class ListItemDelegate : public QStyledItemDelegate
{
public:
	explicit ListItemDelegate(int itemHieght = 36, bool enableMultiLevel = false, QObject* parent = nullptr)
		: QStyledItemDelegate(parent), m_itemHeight(itemHieght), m_enableMultiLevel(enableMultiLevel)
	{
		// 选中的背景颜色
		QColor color = DesignSystem::instance()->primaryColor();
		color.setAlpha(60);
		m_curIdxBgColor = color;
		// 获取默认的背景色
		m_bgColor = DesignSystem::instance()->currentTheme().popupItemBgColor;
		// 获取默认的文本颜色
		m_textColor = DesignSystem::instance()->currentTheme().popupTextColor;
	}

	// 重写 sizeHint 来指定每个 item 的大小
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		// 获取原始的建议大小
		QSize originalSize = QStyledItemDelegate::sizeHint(option, index);
		// 设置一个固定的、更大的高度，例如 36px。你可以根据需要调整。
		return QSize(originalSize.width(), m_itemHeight);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		painter->save();

		QRectF rect = option.rect.adjusted(2, 2, -2, -2);
		int borderRadius = 6;

		const QListView* listView = qobject_cast<const QListView*>(option.widget);
		QModelIndex currentIndex = listView ? listView->currentIndex() : QModelIndex();

		bool isHovered = option.state & QStyle::State_MouseOver;
		bool isCurrent = index == currentIndex;

		// 画背景
		if (isCurrent)
		{
			painter->setBrush(m_curIdxBgColor);
			painter->setPen(Qt::NoPen);
			painter->drawRoundedRect(rect, borderRadius, borderRadius);
		}
		else if (isHovered)
		{
			painter->setBrush(m_bgColor);
			painter->setPen(Qt::NoPen);
			painter->drawRoundedRect(rect, borderRadius, borderRadius);
		}

		// 设置字体
		QFont font = option.font;
		if (isCurrent)
		{
			font.setBold(true);
		}
		painter->setFont(font);

		// 字体颜色
		painter->setPen(m_textColor);

		// 文本区域预留出右侧图标空间
		QRect textRect = option.rect.adjusted(10, 0, -10, 0);
		// 画文本
		QString text = index.data(Qt::DisplayRole).toString();
		int flags = m_enableMultiLevel ? (Qt::AlignVCenter | Qt::AlignLeft) : Qt::AlignCenter;
		painter->drawText(textRect, flags, text);

		// 画图标（右侧）
		if (m_enableMultiLevel)
		{
			QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
			if (!icon.isNull())
			{
				int iconSize = 12;
				QRect iconRect(option.rect.right() - iconSize - 5,
					option.rect.top() + (option.rect.height() - iconSize) / 2,
					iconSize, iconSize);
				icon.paint(painter, iconRect);
			}
		}

		painter->restore();
	}

	void updateStyle(QColor curIdxBgColor, QColor bgColor, QColor textColor)
	{
		curIdxBgColor.setAlpha(60);
		m_curIdxBgColor = curIdxBgColor;
		m_bgColor = bgColor;
		m_textColor = textColor;
	}
private:
	int m_itemHeight;			// 用于存储每个 item 的高度
	QColor m_bgColor;			// 默认的背景色
	QColor m_curIdxBgColor;		// 当前选中项的背景颜色
	QColor m_textColor;			// 文本颜色
	bool m_enableMultiLevel;	// 是否启用多级列表
};

class ListViewStyle : public QProxyStyle
{
public:
	explicit ListViewStyle(QColor bgColor, QColor borderColor, QColor shadowColor, QStyle* baseStyle = nullptr)
		: QProxyStyle(baseStyle), m_bgColor(bgColor), m_borderColor(borderColor), m_shadowColor(shadowColor)
	{
	}

	void drawControl(ControlElement element, const QStyleOption* option,
		QPainter* painter, const QWidget* widget) const override
	{
		if (element == CE_ShapedFrame && widget && qobject_cast<const QListView*>(widget))
		{
			painter->save();
			painter->setRenderHint(QPainter::Antialiasing);

			// 先绘制阴影
			int spread = 6;
			int baseAlpha = 50;
			int radius = 6;

			QRect rect = option->rect.adjusted(spread, spread, -spread, -spread);

			for (int i = 0; i < spread; ++i)
			{
				int alpha = baseAlpha * (1.0f - static_cast<float>(i) / spread);
				QColor shadow = m_shadowColor;
				shadow.setAlpha(alpha);
				QPen pen(shadow, 1.0);
				painter->setPen(pen);
				painter->setBrush(Qt::NoBrush);
				QRect shadowRect = rect.adjusted(-i, -i, i, i);
				painter->drawRoundedRect(shadowRect, radius + i, radius + i);
			}

			// 再绘制背景和边框
			painter->setPen(QPen(m_borderColor, 1.5));
			painter->setBrush(m_bgColor);
			painter->drawRoundedRect(rect, radius, radius);

			painter->restore();
			return;
		}
		QProxyStyle::drawControl(element, option, painter, widget);
	}
	void updateStyle(QColor bgColor, QColor borderColor, QColor shadowColor)
	{
		m_bgColor = bgColor;
		m_borderColor = borderColor;
		m_shadowColor = shadowColor;
	}
private:
	QColor m_bgColor;
	QColor m_borderColor;
	QColor m_shadowColor;
};

class PopupWidget : public QListView
{
	Q_OBJECT

public:
	explicit PopupWidget(int height, bool enableMultiLevel, QWidget* parent = nullptr);
	// 用于设置当前项
	void setCurrentIndex(const QModelIndex& index);
signals:
	void itemSelected(const QModelIndex& index);
private:
	ListViewStyle* m_style;
	ListItemDelegate* m_itmeDele;
	bool m_enableMultiLevel;	// 是否启用多级列表
	int m_popupHeight;			// 弹出框的高度
};
