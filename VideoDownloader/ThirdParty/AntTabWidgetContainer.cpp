#include "AntTabWidgetContainer.h"
#include <QVBoxLayout>
#include <QStackedWidget>
#include "TabContentWidget.h"

AntTabWidgetContainer::AntTabWidgetContainer(int tabwidth, int tabHeight, bool isEnableAddBtn, QWidget* parent)
	: QWidget(parent),
	m_tabBar(new AntTabBarWidget(tabwidth, tabHeight, isEnableAddBtn, this)),
	m_contentWidget(new QStackedWidget(this))
{
	setObjectName("AntTabWidgetContainer");
	setAttribute(Qt::WA_TranslucentBackground);

	// 布局
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_tabBar);
	layout->addWidget(m_contentWidget);

	connect(m_tabBar, &AntTabBarWidget::tabClicked, this, &AntTabWidgetContainer::onTabClicked);

	connect(m_tabBar, &AntTabBarWidget::addTabAndContent, this, [this](QString title)
		{
			m_tabBar->addTab(title, ":/Imgs/git.svg");
			TabContentWidget* contentWidget = new TabContentWidget("标签内容" + title, this);
			m_tabsContent.append(contentWidget);
			m_contentWidget->addWidget(contentWidget);
			m_contentWidget->setCurrentWidget(contentWidget);
		});

	connect(m_tabBar, &AntTabBarWidget::removeContent, this, [this](int index, int currentIndex)
		{
			if (index >= 0 && index < m_contentWidget->count())
			{
				QWidget* widget = m_contentWidget->widget(index);

				m_contentWidget->removeWidget(widget);  // 从堆叠窗口中移除
				widget->deleteLater();					// 异步安全删除
				m_contentWidget->setCurrentIndex(currentIndex);
			}
		});
}

void AntTabWidgetContainer::addTab(const QString& title, const QString& icon, QWidget* contentWidget)
{
	m_tabBar->addTab(title, icon);
	m_tabsContent.append(contentWidget);
	m_contentWidget->addWidget(contentWidget);
	m_contentWidget->setCurrentWidget(contentWidget);
}

int AntTabWidgetContainer::currentIndex() const
{
	return m_tabBar->currentIndex();
}

void AntTabWidgetContainer::setCurrentIndex(int index)
{
	m_tabBar->setCurrentIndex(index);
	m_contentWidget->setCurrentIndex(index);
}

void AntTabWidgetContainer::onTabClicked(int index)
{
	// 业务逻辑
	m_contentWidget->setCurrentIndex(index);
}

void AntTabWidgetContainer::resizeEvent(QResizeEvent* event)
{
	// 重设内容区和 tab 的位置
	QWidget::resizeEvent(event);
}