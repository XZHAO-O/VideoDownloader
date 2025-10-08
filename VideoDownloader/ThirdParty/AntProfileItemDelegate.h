#pragma once

#include <QStyledItemDelegate>
#include <QColor>
#include <QPainter>
#include <QFontMetrics>
#include <QVariantList>

class AntProfileTable;

struct TableColumnLayout
{
	int avatarWidth;
	int actionWidth;

	double nameRatio;
	double ageRatio;
	double addrRatio;
	double tagRatio;
};

class AntProfileItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	AntProfileItemDelegate(AntProfileTable* table, const TableColumnLayout& layout, int cellHeight, QObject* parent = nullptr);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	int getCurrentHoverRow() const;
	void setCurrentHoverRow(int row);

	void updateStyle();
signals:
	void itemClicked(const QModelIndex& index);

private:
	int m_cellHeight;
	int m_nameColumn;
	int m_actionColumn;
	int m_currentHoverRow;
	TableColumnLayout m_layout;
	AntProfileTable* m_table;
	QColor m_hoverColor;
	QColor m_textColor;
	QColor m_bgColor;
	QColor m_primaryColor;
};
