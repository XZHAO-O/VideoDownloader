#pragma once

#include <QWidget>
#include <QPushButton>

class RegisterPageWidget : public QWidget
{
	Q_OBJECT

public:
	RegisterPageWidget(QWidget* parent = nullptr);
	~RegisterPageWidget();

	void updateTheme();
	int pageWidth() const { return w; }
	int pageHeight() const { return h; }
public:
	QPushButton* loginButton;
signals:
	void registerSuccess();
private:
	int w;
	int h;
	QWidget* titleWidget;
};
