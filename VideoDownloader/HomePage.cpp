#include "HomePage.h"
#include <QLayout>
#include <QApplication>
#include <QScreen>
#include "DesignSystem.h"
#include "StyleSheet.h"
#include "AntButton.h"

HomePage::HomePage(QSharedPointer<ApplicationController> appController, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
	, videoWindow(nullptr)
	, antInput(nullptr)
	, m_searchResultsWidget(nullptr)
{
	setObjectName("HomePage");
	setupUI();
	setupConnections();

	// 初始隐藏搜索结果组件
	m_searchResultsWidget->hide();
}

HomePage::~HomePage()
{
}

void HomePage::setupUI()
{
	// 创建搜索输入框
	QStringList listItems = {};
	antInput = new AntInput(300, listItems);
	antInput->setFixedWidth(400);
	antInput->setFixedHeight(50);
	antInput->setPlaceholderText("搜索视频内容...");

	// 创建搜索结果组件
	m_searchResultsWidget = new SearchResultsWidget(this);

	// 使用网格布局实现居中
	QGridLayout* gridLayout = new QGridLayout(this);
	gridLayout->setContentsMargins(0, 0, 0, 0);
	gridLayout->setSpacing(0);

	// 将输入框放在网格的中心位置
	gridLayout->addWidget(antInput, 1, 1);

	// 设置行列的伸缩因子，使输入框位于高度的1/4处
	gridLayout->setRowStretch(0, 1);
	gridLayout->setRowStretch(1, 0);
	gridLayout->setRowStretch(2, 3);
	gridLayout->setColumnStretch(0, 1);
	gridLayout->setColumnStretch(1, 0);
	gridLayout->setColumnStretch(2, 1);
}

void HomePage::setupConnections()
{
	connect(antInput, &AntInput::textChanged, this, &HomePage::onSearchTextChanged);
	connect(antInput, &AntInput::returnPressed, this, &HomePage::onSearchClicked);
	connect(m_searchResultsWidget, &SearchResultsWidget::nextButtonClicked, this, &HomePage::onNextButtonClicked);
}

void HomePage::onSearchTextChanged(const QString& text)
{
	Q_UNUSED(text);
}

void HomePage::onSearchClicked()
{
	QString searchText = antInput->text().trimmed();
	if (searchText.isEmpty()) {
		m_searchResultsWidget->hide();
		return;
	}

	// 加载模拟数据
	loadMockSearchData();

	// 显示并定位搜索结果组件
	updateSearchResultsPosition();
	m_searchResultsWidget->show();
	m_searchResultsWidget->raise();
}

void HomePage::onNextButtonClicked()
{
	// 隐藏搜索结果组件
	m_searchResultsWidget->hide();

	// 清空搜索框
	antInput->clear();

	// 发出导航信号
	emit navigateToDownloadRequested();

	qDebug() << "导航到下载队列页面，选中了" << m_searchResultsWidget->getSelectedCount() << "个项目";
}

void HomePage::updateSearchResultsPosition()
{
	if (!m_searchResultsWidget || !antInput) return;

	// 获取搜索框的全局位置
	QPoint inputGlobalPos = antInput->mapToGlobal(QPoint(0, 0));
	// 转换为相对于HomePage的位置
	QPoint inputLocalPos = mapFromGlobal(inputGlobalPos);

	// 设置搜索结果组件的位置和大小
	int containerWidth = antInput->width();
	int containerHeight = static_cast<int>(height() * 0.6);

	m_searchResultsWidget->setFixedSize(containerWidth, containerHeight);
	m_searchResultsWidget->move(inputLocalPos.x(),
		inputLocalPos.y() + antInput->height() + 8);
}

void HomePage::loadMockSearchData()
{
	// 清空之前的结果
	m_searchResultsWidget->clearAll();

	// 模拟搜索数据
	QStringList mockTitles = {
		"Qt编程入门教程 - 从零开始学习GUI开发",
		"C++高级编程技巧与最佳实践",
		"设计模式在Qt中的应用实例",
		"多线程编程实战指南",
		"现代C++新特性详解"
	};

	QStringList mockDurations = { "12:34", "23:45", "45:12", "34:56", "56:78" };
	QStringList mockAuthors = { "程序员老王", "技术达人", "代码艺术家", "架构师之路", "编程思维" };

	for (int i = 0; i < mockTitles.size(); ++i) {
		m_searchResultsWidget->addSearchResultItem(mockTitles[i], mockDurations[i], mockAuthors[i]);
	}
}

void HomePage::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
}

void HomePage::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	updateSearchResultsPosition();
}