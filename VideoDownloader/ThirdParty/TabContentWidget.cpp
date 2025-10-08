#include "TabContentWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFont>
#include "DesignSystem.h"

TabContentWidget::TabContentWidget(const QString& text, QWidget* parent)
	: QWidget(parent)
{
	setStyleSheet(QString("background-color: %1;").arg(DesignSystem::instance()->backgroundColor().name()));

	// 创建标签
	QLabel* label = new QLabel(text, this);
	label->setAlignment(Qt::AlignCenter);

	// 设置字体
	QFont font;
	font.setBold(true);
	font.setPointSize(25);
	label->setFont(font);

	// 布局管理器
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(label);
	layout->setContentsMargins(0, 0, 0, 0);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			setStyleSheet(QString("background-color: %1;").arg(DesignSystem::instance()->backgroundColor().name()));
		});
}

TabContentWidget::~TabContentWidget()
{
}