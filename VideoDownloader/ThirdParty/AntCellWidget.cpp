#include "AntCellWidget.h"
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QApplication>
#include "DesignSystem.h"
#include "StyleSheet.h"

AntCellWidget::AntCellWidget(const QString& btnName, QWidget* parent)
	: QWidget(parent)
{
	// 根据业务需求调整内容

	// 为 AntCellWidget 安装事件过滤器
	this->installEventFilter(this);
	this->setMouseTracking(true);

	auto theme = DesignSystem::instance()->currentTheme();

	// 创建名字按钮
	button = new QPushButton(btnName, this);
	button->setCursor(Qt::PointingHandCursor);
	button->setStyleSheet(StyleSheet::textBtnQss("left", theme.primaryColor, theme.primaryHoverColor));
	button->setMinimumWidth(50);

	button->setAttribute(Qt::WA_TransparentForMouseEvents, false);

	// 在按钮点击时执行相关操作
	connect(button, &QPushButton::clicked, this, [=]()
		{
			// 处理点击事件
		});

	// 创建一个水平布局来包裹按钮
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(8, 0, 0, 0);		// 设置左边距 8px
	layout->addWidget(button);
	layout->addStretch();

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			auto theme = DesignSystem::instance()->currentTheme();
			button->setStyleSheet(StyleSheet::textBtnQss("left", theme.primaryColor, theme.primaryHoverColor));
		});
}

AntCellWidget::~AntCellWidget()
{
}

// 事件过滤器实现
bool AntCellWidget::eventFilter(QObject* watched, QEvent* event)
{
	// 只处理鼠标事件
	if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
	{
		// 如果是按钮，转发事件
		if (watched == button)
		{
			// 将事件转发给按钮
			QCoreApplication::sendEvent(button, event);
			return true;  // 已处理
		}
		else if (watched == this)
		{
			QCoreApplication::sendEvent(parent(), event);
			return true;
		}
		else
		{
			// 否则，忽略事件
			return true;
		}
	}

	// 默认处理其他事件
	return QWidget::eventFilter(watched, event);
}