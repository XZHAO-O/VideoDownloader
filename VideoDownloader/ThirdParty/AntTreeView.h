#pragma once

#include <QTreeView>
#include <QVariantAnimation>
#include <QEvent>
#include <QMouseEvent>
#include <QSvgRenderer>
#include <QPainterPath>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QModelIndex>
#include <QStandardItemModel>
#include "DesignSystem.h"

class AntTreeItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	AntTreeItemDelegate(int minimumHeight, int arrowSize, int iconSize, int indent, QObject* parent = nullptr)
		: QStyledItemDelegate(parent), m_minimumHeight(minimumHeight), m_arrowSize(arrowSize), m_iconSize(iconSize), m_indent(indent)
	{
		m_hoverColor = DesignSystem::instance()->widgetHoverBgColor();
		m_hoverColor.setAlpha(80);
		svgRenderer = new QSvgRenderer(QStringLiteral(":/Imgs/smallRightArrow.svg"), this);
	}

	void setRotationMap(const QMap<QModelIndex, int>& rotations)
	{
		m_rotations = rotations;
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		QStyleOptionViewItem opt(option);
		initStyleOption(&opt, index);

		// 放大字体 1pt
		opt.font.setPointSizeF(opt.font.pointSizeF() + 1);

		bool isHovered = (index == hoverIndex);

		QColor backgroundColor = DesignSystem::instance()->backgroundColor();
		if (isHovered)
		{
			backgroundColor = m_hoverColor;
		}

		painter->save();
		painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing
			| QPainter::SmoothPixmapTransform | QPainter::LosslessImageRendering);

		// 画圆角背景（选中或悬浮）
		QRect rect = option.rect.adjusted(4, 2, -4, -2);
		if (isHovered)
		{
			QPainterPath path;
			path.addRoundedRect(rect, 4, 4);
			painter->fillPath(path, backgroundColor);
		}

		// 只有有子节点的才绘制箭头
		if (index.model()->hasChildren(index))
		{
			// 先画箭头，箭头大小和图标大小差不多，放在图标左侧，预留8像素间隔
			int level = 0;
			QModelIndex parent = index.parent();
			while (parent.isValid())
			{
				level++;
				parent = parent.parent();
			}
			int xOffset = level * m_indent; // 或者你自己定义的缩进单位

			QRect arrowRect(rect.left() + xOffset + 8, rect.top() + (rect.height() - m_arrowSize) / 2, m_arrowSize, m_arrowSize);
			int rotation = m_rotations.value(index, 0);

			// 旋转绘制箭头
			painter->save();
			QPoint center = arrowRect.center();
			painter->translate(center);
			painter->rotate(rotation);
			painter->translate(-center);
			svgRenderer->render(painter, arrowRect);
			painter->restore();

			// 图标紧跟箭头
			QRect iconRect(arrowRect.right() + 8, rect.top() + (rect.height() - m_iconSize) / 2, m_iconSize, m_iconSize);
			QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
			if (!icon.isNull())
				icon.paint(painter, iconRect, Qt::AlignCenter);

			// 文本区
			QRect textRect = rect.adjusted(xOffset + m_arrowSize + m_iconSize + 24, 0, 0, 0);
			painter->setFont(opt.font);
			painter->setPen(DesignSystem::instance()->currentTheme().listTextColor);
			painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, index.data(Qt::DisplayRole).toString());
		}
		else
		{
			// 没有子节点，画图标+文本，图标靠左，间隔8像素
			int level = 0;
			QModelIndex parent = index.parent();
			while (parent.isValid())
			{
				level++;
				parent = parent.parent();
			}
			int xOffset = m_indent * level;

			// 没有子节点，画图标+文本，图标靠左，间隔8像素
			QRect iconRect(rect.left() + 8 + xOffset, rect.top() + (rect.height() - m_iconSize) / 2, m_iconSize, m_iconSize);
			QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
			if (!icon.isNull())
			{
				icon.paint(painter, iconRect, Qt::AlignCenter);
			}

			QRect textRect = rect.adjusted(xOffset + m_iconSize + 16, 0, 0, 0);
			painter->setFont(opt.font);
			painter->setPen(DesignSystem::instance()->currentTheme().listTextColor);
			painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, index.data(Qt::DisplayRole).toString());
		}

		painter->restore();
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		// 动态计算高度
		QFont font = option.font;
		QFontMetrics fm(font);
		int textHeight = fm.height();

		int contentHeight = qMax(textHeight, m_iconSize);

		// 添加上下边距
		int finalHeight = contentHeight + 10;

		return QSize(0, qMax(finalHeight, m_minimumHeight));
	}

	void updateStyle()
	{
		m_hoverColor = DesignSystem::instance()->widgetHoverBgColor();
		m_hoverColor.setAlpha(80);
	}
public:
	QModelIndex hoverIndex; // 当前悬浮的 index
private:
	QColor m_hoverColor;
	int m_minimumHeight;
	int m_iconSize;
	int m_arrowSize;
	int m_indent;	// 缩进自己调整
	QMap<QModelIndex, int> m_rotations;
	QSvgRenderer* svgRenderer;
};

class AntTreeView : public QTreeView
{
	Q_OBJECT

public:
	struct ItemData {
		QString text;
		QIcon icon;
		QList<ItemData> children; // 递归子项
	};

	AntTreeView(int itemHeight, QWidget* parent);
	~AntTreeView();
	QStandardItemModel* createModel(const QList<ItemData>& data);
	// 计算全部展开后的总高度
	void calculateFullHeight();
protected:
	bool viewportEvent(QEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void leaveEvent(QEvent* event) override;
private:
	void startArrowAnimation(const QModelIndex& index, int target);
private:
	int m_itemHeight;
	int m_arrowRotation;
	QVariantAnimation m_animation;
	AntTreeItemDelegate* m_delegate;
	// 用QHash维护所有item的箭头角度
	QMap<QModelIndex, int> m_arrowRotations;
	QModelIndex m_currentAnimationIndex; // 正在动画的节点索引
};
