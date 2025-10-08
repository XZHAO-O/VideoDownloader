#include "AntBaseTableView.h"

AntBaseTableView::AntBaseTableView(QWidget* parent)
	: QTableView(parent)
{
	initStyle();
}

void AntBaseTableView::initStyle()
{
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setShowGrid(false);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setStyleSheet("QTableView { border: none; }");
}