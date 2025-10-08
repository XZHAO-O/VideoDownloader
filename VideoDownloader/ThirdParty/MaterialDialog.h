#pragma once

#include <QWidget>
#include <QEvent>
#include <QStackedWidget>
#include "MaskWidget.h"
#include "LoginPageWidget.h"
#include "RegisterPageWidget.h"
#include "SlideStackedWidget.h"
#include "ProfilePage.h"
#include "StandardDialogPage.h"

class MaterialDialog : public QWidget
{
	Q_OBJECT

public:
	enum PageIndex
	{
		Login,
		Register,
		Profile,
		Standard
	};

	MaterialDialog(bool loginState, std::function<void(MaterialDialog::PageIndex)> callback, QWidget* parent);
	~MaterialDialog();
public:
	void showIndexPage(PageIndex index);
	StandardDialogPage* standardDialog() { return standardPage; };
signals:
	void setStandardDialogText(QString title, QString text);
	void successLogin(bool loginState);
public:
	LoginPageWidget* loginPage;
	RegisterPageWidget* registerPage;
	ProfilePage* profilePage;
	StandardDialogPage* standardPage;
private:
	// 用户登录状态
	bool m_loginState = false;
	// 页面管理
	SlideStackedWidget* stackedWidget;
	std::function<void(MaterialDialog::PageIndex)> m_callback;
};
