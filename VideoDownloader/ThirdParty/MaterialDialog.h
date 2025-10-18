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
#include "QRCodeLoginWidget.h"

class MaterialDialog : public QWidget
{
	Q_OBJECT

public:
	enum PageIndex
	{
		Login,
		Register,
		Profile,
		Standard,
		QRCodeLogin
	};

	MaterialDialog(bool loginState, std::function<void(MaterialDialog::PageIndex)> callback, QWidget* parent);
	~MaterialDialog();

	void showIndexPage(PageIndex index);
	StandardDialogPage* standardDialog() { return standardPage; };

	// 新增：设置标准对话框确认函数
	void setStandardConfirmFunction(std::function<void()> func);

	void setQRCodeLoginWidgetContent(const QString& title, QWidget* qrCodeLoginWidget);
	void showQRCodeLoginWidget();

signals:
	void setStandardDialogText(QString title, QString text);
	void successLogin(bool loginState);

public:
	LoginPageWidget* loginPage;
	RegisterPageWidget* registerPage;
	ProfilePage* profilePage;
	StandardDialogPage* standardPage;
	QRCodeLoginWidget* qrCodeLoginWidget;
private:
	// 用户登录状态
	bool m_loginState = false;
	// 页面管理
	SlideStackedWidget* stackedWidget;
	std::function<void(MaterialDialog::PageIndex)> m_callback;
};