#pragma once

#include <QWidget>
#include <QPushButton>

class LoginPageWidget : public QWidget
{
	Q_OBJECT

public:
	LoginPageWidget(QWidget* parent = nullptr);
	~LoginPageWidget();

	void updateTheme();
	int pageWidth() const { return w; } // 页面宽度
	int pageHeight() const { return h; } // 页面高度
public:
	QPushButton* registerButton;
signals:
	void loginSuccess();
private:
	int w;
	int h;
	QWidget* titleWidget;
	QPushButton* subBtn2;
};
