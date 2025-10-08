#pragma once

#include <QWidget>

class ProfilePage : public QWidget
{
	Q_OBJECT

public:
	ProfilePage(QWidget* parent = nullptr);
	~ProfilePage();

	int pageWidth() const { return w; } // 页面宽度
	int pageHeight() const { return h; } // 页面高度
private:
	int w = 600;
	int h = 400;
};
