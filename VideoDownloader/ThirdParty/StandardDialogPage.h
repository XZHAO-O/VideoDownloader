#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <functional>

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

	// 新增：设置确认按钮的自定义函数
	void setConfirmFunction(std::function<void()> func);

signals:
	void exitDialog();
	void confirmClicked();  // 新增信号

private:
	int w;
	int h;
	QLabel* titleLabel;
	QLabel* contentLabel;
	QPushButton* cancelBtn;
	QPushButton* confirmBtn;
	std::function<void()> m_confirmFunction;  // 存储自定义函数
};