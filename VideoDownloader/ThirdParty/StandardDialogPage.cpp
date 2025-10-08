#include "StandardDialogPage.h"
#include "StyleSheet.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QScrollArea>
#include <QFrame>
#include <QApplication>
#include "DesignSystem.h"

StandardDialogPage::StandardDialogPage(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("standardDialog");
	setStyleSheet(StyleSheet::standardDialogQss());

	// 主垂直布局
	QVBoxLayout* mainLay = new QVBoxLayout(this);
	mainLay->setContentsMargins(20, 20, 20, 20);
	mainLay->setSpacing(20);

	// ========== 1. 标题栏 ==========
	titleLabel = new QLabel("TITLE");
	QFont font;
	font.setPointSize(14);
	font.setBold(true);
	titleLabel->setFont(font);
	titleLabel->setFixedHeight(30);

	// ========== 2. 内容区（可滚动） ==========
	contentLabel = new QLabel("?");
	font.setPointSize(12.5);
	font.setBold(false);

	// ========== 3. 底部操作区 ==========
	QHBoxLayout* buttonLayout = new QHBoxLayout;
	buttonLayout->setSpacing(12);
	font.setPointSize(11.5);
	font.setBold(false);
	cancelBtn = new QPushButton("取消");
	confirmBtn = new QPushButton("确定");
	cancelBtn->setCursor(Qt::PointingHandCursor);
	confirmBtn->setCursor(Qt::PointingHandCursor);
	cancelBtn->setFixedSize(60, 32);
	confirmBtn->setFixedSize(60, 32);

	cancelBtn->setFont(font);
	confirmBtn->setFont(font);

	cancelBtn->setObjectName("CancelButton");
	confirmBtn->setObjectName("ConfirmButton");
	cancelBtn->setStyleSheet(StyleSheet::standardDialogBtnQss(DesignSystem::instance()->primaryColor(), DesignSystem::instance()->backgroundColor()));
	confirmBtn->setStyleSheet(StyleSheet::standardDialogBtnQss(DesignSystem::instance()->primaryColor(), DesignSystem::instance()->backgroundColor()));

	buttonLayout->addStretch();			// 推到右侧
	buttonLayout->addWidget(cancelBtn);
	buttonLayout->addWidget(confirmBtn);

	mainLay->addWidget(titleLabel);
	mainLay->addWidget(contentLabel);
	mainLay->addLayout(buttonLayout);

	// ===== 连接信号和槽：点击“确定”退出程序 =====
	connect(confirmBtn, &QPushButton::clicked, qApp, &QApplication::quit);
	connect(cancelBtn, &QPushButton::clicked, this, &StandardDialogPage::exitDialog);

	adjustSize();

	h = size().height() * 1.15;
	w = h * 1.8;
}

StandardDialogPage::~StandardDialogPage()
{
}

void StandardDialogPage::setText(QString title, QString text)
{
	if (titleLabel && contentLabel)
	{
		titleLabel->setText(title);
		contentLabel->setText(text);
	}
}

void StandardDialogPage::updateTheme()
{
	cancelBtn->setStyleSheet(StyleSheet::standardDialogBtnQss(DesignSystem::instance()->primaryColor(), DesignSystem::instance()->backgroundColor()));
	confirmBtn->setStyleSheet(StyleSheet::standardDialogBtnQss(DesignSystem::instance()->primaryColor(), DesignSystem::instance()->backgroundColor()));
}