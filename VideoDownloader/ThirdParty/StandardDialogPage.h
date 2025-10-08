#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class StandardDialogPage : public QWidget
{
	Q_OBJECT

public:
	StandardDialogPage(QWidget* parent);
	~StandardDialogPage();

	void setText(QString title, QString text);
	void updateTheme();
	int pageWidth() { return w; }
	int pageHeight() { return h; }
signals:
	void exitDialog();
private:
	int w;
	int h;
	QLabel* titleLabel;
	QLabel* contentLabel;
	QPushButton* cancelBtn;
	QPushButton* confirmBtn;
};
