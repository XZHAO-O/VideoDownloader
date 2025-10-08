#include "RegisterPageWidget.h"
#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include "StyleSheet.h"
#include "MaterialLineEdit.h"
#include "ErrorTipLabel.h"
#include "AntButton.h"
#include "LogoWidget.h"

RegisterPageWidget::RegisterPageWidget(QWidget* parent)
	: QWidget(parent)
{
	// 主布局
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// 标题
	titleWidget = new QWidget(this);
	titleWidget->setObjectName("dialogSubTitle");
	titleWidget->setStyleSheet(StyleSheet::titleBottomLineQss(DesignSystem::instance()->borderColor()));
	QHBoxLayout* titleLay = new QHBoxLayout(titleWidget);
	QLabel* titleLab = new QLabel("TITLE", titleWidget);
	QFont font;
	font.setBold(true);
	font.setPointSizeF(13.5);
	titleLab->setFont(font);
	titleLay->addSpacing(10);
	titleLay->addWidget(titleLab);
	titleLay->addStretch();

	// 中心
	QHBoxLayout* centralLay = new QHBoxLayout();
	// 左边
	QWidget* leftWidget = new QWidget(this);
	QVBoxLayout* leftLay = new QVBoxLayout(leftWidget);
	leftLay->setSpacing(2);
	// 右边
	QWidget* rightWidget = new QWidget(this);
	QVBoxLayout* rightLay = new QVBoxLayout(rightWidget);
	// sub1
	font.setPointSizeF(11);
	font.setBold(false);
	QWidget* subWidget1 = new QWidget(leftWidget);
	QHBoxLayout* subLay1 = new QHBoxLayout(subWidget1);
	QLabel* subLab1 = new QLabel("邮箱注册", subWidget1);
	loginButton = new QPushButton("登录", subWidget1);
	subLab1->setFont(font);
	loginButton->setFont(font);
	loginButton->setCursor(Qt::PointingHandCursor);
	loginButton->setStyleSheet(StyleSheet::noBorderBtnQss(DesignSystem::instance()->primaryColor()));
	// 布局
	centralLay->setContentsMargins(9, 9, 9, 9);
	leftLay->setContentsMargins(9, 9, 9, 9);
	rightLay->setContentsMargins(9, 9, 9, 30);
	subLay1->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(titleWidget, 2);
	mainLayout->addLayout(centralLay, 5);
	centralLay->addWidget(leftWidget, 3);
	centralLay->addWidget(rightWidget, 2);
	subLay1->addWidget(subLab1);
	subLay1->addStretch();
	subLay1->addWidget(loginButton);
	// 拼界面
	font.setPointSizeF(11.5);
	font.setBold(false);
	// 邮箱
	MaterialLineEdit* accountEdit = new MaterialLineEdit(leftWidget);
	accountEdit->setFont(font);
	accountEdit->setLabelText("邮箱");
	accountEdit->setTextFontSize(10.2);
	accountEdit->setFixedHeight(40);  // 高度可自定义
	// 验证码
	MaterialLineEdit* codeEdit = new MaterialLineEdit(leftWidget);
	codeEdit->setFont(font);
	codeEdit->setLabelText("验证码");
	codeEdit->setTextFontSize(10.2);
	codeEdit->setFixedHeight(40);
	codeEdit->setRightTextBtn("发送验证码");
	// 密码
	MaterialLineEdit* passwordEdit = new MaterialLineEdit(leftWidget);
	passwordEdit->setFont(font);
	passwordEdit->setLabelText("密码");
	passwordEdit->setTextFontSize(10.2);
	passwordEdit->setFixedHeight(40);
	passwordEdit->setPasswordToggleEnabled(leftWidget);
	// 校验失败时调用
	QVector<ErrorTipLabel*>errorTips;
	for (int i = 0; i < 3; ++i)
	{
		ErrorTipLabel* errorTip = new ErrorTipLabel(leftWidget);
		errorTip->setFixedHeight(20);
		errorTips.append(errorTip);
	}
	connect(codeEdit->textBtn(), &QPushButton::clicked, this, [this, codeEdit, errorTips]()
		{
			codeEdit->errorHint();
			errorTips[1]->showError("验证码无法发送!");
		});
	// 按钮
	AntButton* antBtn = new AntButton("注册", 11.5, leftWidget);
	antBtn->setMinimumHeight(50);
	connect(antBtn, &AntButton::clicked, this, [this, errorTips, accountEdit, codeEdit, passwordEdit]()
		{
			bool hasError = false;

			if (accountEdit->text().isEmpty())
			{
				accountEdit->errorHint();
				errorTips[0]->showError("邮箱为空!");
				hasError = true;
			}
			else
			{
				errorTips[0]->clearHintText();  // 清除提示
			}

			if (codeEdit->text().isEmpty())
			{
				codeEdit->errorHint();
				errorTips[1]->showError("验证码为空!");
				hasError = true;
			}
			else
			{
				errorTips[1]->clearHintText();  // 清除提示
			}

			if (passwordEdit->text().isEmpty())
			{
				passwordEdit->errorHint();
				errorTips[2]->showError("密码为空!");
				hasError = true;
			}
			else
			{
				errorTips[2]->clearHintText();  // 同上
			}

			if (!hasError)
			{
				emit registerSuccess();
				accountEdit->clear();
				codeEdit->clear();
				passwordEdit->clear();
			}
		});

	// 布局
	leftLay->addWidget(subWidget1);
	leftLay->addSpacing(5);
	leftLay->addWidget(accountEdit);
	leftLay->addWidget(errorTips[0]);
	leftLay->addWidget(codeEdit);
	leftLay->addWidget(errorTips[1]);
	leftLay->addWidget(passwordEdit);
	leftLay->addWidget(errorTips[2]);
	leftLay->addWidget(antBtn);
	leftLay->addStretch();

	// LOGO
	LogoWidget* logo = new LogoWidget(rightWidget);
	// 因为是圆形 宽高一定要相同
	rightLay->addWidget(logo);
	logo->show();

	adjustSize();

	h = size().height();
	w = (size().height() + 10) * 1.5;
}

RegisterPageWidget::~RegisterPageWidget()
{
}

void RegisterPageWidget::updateTheme()
{
	titleWidget->setStyleSheet(StyleSheet::titleBottomLineQss(DesignSystem::instance()->borderColor()));
	loginButton->setStyleSheet(StyleSheet::noBorderBtnQss(DesignSystem::instance()->primaryColor()));
}