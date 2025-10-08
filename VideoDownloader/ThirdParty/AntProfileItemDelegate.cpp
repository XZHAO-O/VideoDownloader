#include "AntProfileTable.h"
#include "DesignSystem.h"

AntProfileItemDelegate::AntProfileItemDelegate(AntProfileTable* table, const TableColumnLayout& layout, int cellHeight, QObject* parent)
	: QStyledItemDelegate(parent), m_table(table), m_layout(layout), m_cellHeight(cellHeight),
	m_nameColumn(1), m_actionColumn(5), m_currentHoverRow(-1)
{
	m_bgColor = DesignSystem::instance()->backgroundColor();
	m_textColor = DesignSystem::instance()->currentTheme().tableTextColor;
	m_primaryColor = DesignSystem::instance()->primaryColor();
	m_hoverColor = DesignSystem::instance()->currentTheme().widgetHoverBgColor;
	m_hoverColor.setAlpha(80);
}

void AntProfileItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	// 6 列：头像、名字、年龄、地址、标签、操作
	painter->save();
	painter->setRenderHints(QPainter::Antialiasing | QPainter::LosslessImageRendering | QPainter::SmoothPixmapTransform);

	// 绘制背景色
	if (m_table->selectionBehavior() == QAbstractItemView::SelectRows &&
		index.row() == m_currentHoverRow)
	{
		painter->setPen(Qt::NoPen);
		painter->setBrush(m_hoverColor);
		painter->drawRect(option.rect);
	}
	else
	{
		painter->setPen(Qt::NoPen);
		painter->setBrush(m_bgColor);
		painter->drawRect(option.rect);
	}

	const int col = index.column();

	if (col == 0)
	{
		// 头像列
		QPixmap avatar = index.data(Qt::DecorationRole).value<QPixmap>();
		if (!avatar.isNull())
		{
			int avatarSize = m_layout.avatarWidth - 16; // 留点边距
			QRect avatarRect(
				option.rect.left() + 8,
				option.rect.top() + (option.rect.height() - avatarSize) / 2,
				avatarSize,
				avatarSize
			);
			painter->drawPixmap(avatarRect, avatar);
		}
	}
	else if (col == 2 || col == 3)
	{
		// 年龄 / 地址：左对齐文字
		QString text = index.data(Qt::DisplayRole).toString();
		painter->setPen(m_textColor);
		painter->drawText(option.rect.adjusted(8, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
	}
	else if (col == 4)
	{
		// 标签列（多个彩色标签）
		QVariantList tagList = index.data(Qt::UserRole + 1).toList();
		int x = option.rect.left() + 8;
		int tagHeight = 26;
		int y = option.rect.center().y() - tagHeight / 2;

		for (const QVariant& v : tagList)
		{
			QVariantMap map = v.toMap();
			QString text = map["text"].toString();
			QColor color = map["color"].value<QColor>();

			QSize textSize = QFontMetrics(painter->font()).size(0, text);
			QRect tagRect(x, y, textSize.width() + 10, tagHeight);
			painter->setPen(Qt::NoPen);
			painter->setBrush(color.isValid() ? color : Qt::lightGray);
			painter->drawRoundedRect(tagRect, 4, 4);
			painter->setPen(DesignSystem::instance()->textColor());
			painter->drawText(tagRect, Qt::AlignCenter, text);

			x += tagRect.width() + 8;
		}
	}

	painter->restore();
}

QSize AntProfileItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	return QSize(option.rect.width(), m_cellHeight);
}

int AntProfileItemDelegate::getCurrentHoverRow() const
{
	return m_currentHoverRow;
}

void AntProfileItemDelegate::setCurrentHoverRow(int row)
{
	m_currentHoverRow = row;
}

void AntProfileItemDelegate::updateStyle()
{
	m_bgColor = DesignSystem::instance()->backgroundColor();
	m_textColor = DesignSystem::instance()->currentTheme().tableTextColor;
	m_primaryColor = DesignSystem::instance()->primaryColor();
	m_hoverColor = DesignSystem::instance()->currentTheme().widgetHoverBgColor;
	m_hoverColor.setAlpha(80);
}