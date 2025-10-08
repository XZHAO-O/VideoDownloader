#pragma once

#include "AntBaseTableView.h"
#include <QHeaderView>
#include <QEvent>
#include <QStandardItemModel>
#include <QProxyStyle>
#include <QStyleOptionViewItem>
#include <QVector>
#include "AntProfileItemDelegate.h"

class AntBaseHeaderView : public QHeaderView
{
public:
	explicit AntBaseHeaderView(Qt::Orientation orientation, int headerHeight, QWidget* parent = nullptr)
		: QHeaderView(orientation, parent), m_headerHeight(headerHeight)
	{
		m_headerBgColor = DesignSystem::instance()->currentTheme().widgetBgColor;
		m_textColor = DesignSystem::instance()->currentTheme().tableTextColor;
	}

	void updateBaseStyle()
	{
		m_headerBgColor = DesignSystem::instance()->currentTheme().widgetBgColor;
		m_textColor = DesignSystem::instance()->currentTheme().tableTextColor;
	}
protected:
	QSize sizeHint() const override
	{
		QSize size = QHeaderView::sizeHint();
		size.setHeight(m_headerHeight);
		return size;
	}

	void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override
	{
		painter->save();

		// 自定义背景色
		painter->fillRect(rect, m_headerBgColor);

		// 自定义字体
		QFont font = this->font();
		font.setBold(true);
		font.setPointSizeF(font.pointSizeF() + 0.5);
		painter->setFont(font);

		// 自定义文字颜色
		painter->setPen(m_textColor);

		// 获取标题文字
		QString text = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();

		// 设置文本绘制区域
		QRect textRect = rect.adjusted(8, 0, -8, 0);  // 保持左右边距一致

		// 对齐方式：第 0 列居中，其它列左对齐
		int alignment = (logicalIndex == 0)
			? (Qt::AlignVCenter | Qt::AlignHCenter)
			: (Qt::AlignVCenter | Qt::AlignLeft);

		painter->drawText(textRect, alignment, text);

		painter->restore();
	}
protected:
	QColor m_headerBgColor;
	QColor m_textColor;
	int m_headerHeight; // 外部传入的表头高度
};

class AntProfileTableStyle : public QProxyStyle
{
public:
	AntProfileTableStyle(QStyle* style)
		: QProxyStyle(style)
	{
	}

	void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option,
		QPainter* painter, const QWidget* widget) const override
	{
		if (element == QStyle::PE_PanelItemViewRow)
		{
			if (const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>(option))
			{
				if (vopt->state.testFlag(QStyle::State_Selected))
				{
					// 不绘制任何背景，隐藏默认蓝色背景
					return;
				}
			}
		}
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}
};

class AntProfileTable : public AntBaseTableView
{
	Q_OBJECT

public:
	struct TagItem
	{
		QString text;
		QColor color;
	};

	struct TableColumnItems
	{
		QPixmap avatar;      // 头像
		QString name;        // 名字
		QString age;         // 年龄
		QString address;     // 地址

		QVector<TagItem> tags;  // 多个标签

		QString action;      // 操作（按钮或文本）
	};

	AntProfileTable(int headerHeight, int rowsPerPage, int rowHeight, const TableColumnLayout& layout, QWidget* parent);
	~AntProfileTable();

	// 创建模型
	QStandardItemModel* createModel(const QVector<TableColumnItems>& colItems);
protected:
	void mouseMoveEvent(QMouseEvent* event);
	void leaveEvent(QEvent* event);
	void resizeEvent(QResizeEvent* event);
	void updateColumnWidths();
private:
	TableColumnLayout m_columnLayout;
	AntBaseHeaderView* customHeader = nullptr;
	AntProfileItemDelegate* itemDelegate = nullptr;
	AntProfileTableStyle* tableStyle = nullptr;

	int m_rowsPerPage;
	int m_rowHeight;
	int m_headerHeight;

	int avatarWidth = 60;
	int actionWidth = 130;

	double nameRatio = 0.25;
	double ageRatio = 0.10;
	double addrRatio = 0.35;
	double tagRatio = 0.30;
};