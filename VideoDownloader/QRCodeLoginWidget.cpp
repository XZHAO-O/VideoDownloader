#include "QRCodeLoginWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include "StyleSheet.h"
#include "DesignSystem.h"

QRCodeLoginWidget::QRCodeLoginWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("CustomContentPage");

	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(20);
	m_mainLayout->setContentsMargins(10, 10, 10, 10);

	// 标题标签
	m_titleLabel = new QLabel("自定义内容", this);
	m_titleLabel->setAlignment(Qt::AlignCenter);

	m_mainLayout->addStretch();
	m_mainLayout->addWidget(m_titleLabel);
	m_mainLayout->addStretch();
	m_mainLayout->addStretch();

	updateTheme();
}

QRCodeLoginWidget::~QRCodeLoginWidget()
{
}

void QRCodeLoginWidget::setContent(const QString& title, QWidget* customWidget)
{
	// 移除旧的内容部件
	if (m_contentWidget) {
		m_mainLayout->removeWidget(m_contentWidget);
		m_contentWidget->deleteLater();
	}

	// 设置新标题
	m_titleLabel->setText(title);

	// 添加新的内容部件
	m_contentWidget = customWidget;
	h = customWidget->height() * 1.2;
	w = customWidget->width() * 1.2;
	if (m_contentWidget) {
		m_mainLayout->insertWidget(1, m_contentWidget); // 插入到标题和弹性空间之间
	}
}

void QRCodeLoginWidget::updateTheme()
{
	auto theme = DesignSystem::instance()->currentTheme();

	// 设置标题样式
	m_titleLabel->setStyleSheet(QString("QLabel {"
		"font-size: 18px;"
		"font-weight: bold;"
		"color: %1;"
		"background-color: transparent;"
		"border: none;"
		"padding: 10px;"
		"}").arg(theme.primaryTextColor.name()));

	// 设置页面背景
	setStyleSheet(QString("CustomContentPage {"
		"background-color: %1;"
		"border-radius: 8px;"
		"}").arg(theme.backgroundColor.name()));
}