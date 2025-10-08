#pragma once

#include <QWidget>

class TabContentWidget : public QWidget
{
	Q_OBJECT

public:
	TabContentWidget(const QString& text, QWidget* parent);
	~TabContentWidget();
};
