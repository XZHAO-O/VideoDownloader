#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QResizeEvent>

class DrawerWidget : public QWidget
{
	Q_OBJECT

public:
	DrawerWidget(int width, QWidget* parent);
	~DrawerWidget();

	void showDrawer();
	void closeDrawer();
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	QPropertyAnimation* animation = nullptr;
};
