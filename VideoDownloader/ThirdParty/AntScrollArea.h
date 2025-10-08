#pragma once

#include <QScrollArea>
#include <QVBoxLayout>

class AntScrollArea : public QScrollArea
{
	Q_OBJECT

public:
	enum ScrollDirection
	{
		ScrollNone = 0,
		ScrollVertical = 1,
		ScrollHorizontal = 2,
		ScrollBoth = ScrollVertical | ScrollHorizontal
	};

	AntScrollArea(ScrollDirection direction = ScrollVertical, QWidget* parent = nullptr);
	~AntScrollArea();
	void addWidget(QWidget* w);
	void setScrollPolicy(int direction);
	void ensureWidgetVisibleInScrollArea(QWidget* targetWidget, int xMargin = 50, int yMargin = 50);
};
