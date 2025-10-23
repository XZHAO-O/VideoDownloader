#include "HomePage.h"

#include "AntButton.h"
#include "AntInput.h"
#include "ApplicationController.h"
#include "LogSystem.h"
#include "PlatformAggregatorService.h"
#include "SearchResultsWidget.h"
#include "AntMessageManager.h"

HomePage::HomePage(QSharedPointer<ApplicationController> appController, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
	, m_configModManager(appController->getConfigModManager())
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
	connect(antInput, &AntInput::searchClicked, this, &HomePage::onSearchClicked);
	connect(antInput, &AntInput::returnPressed, this, &HomePage::onSearchClicked);
	connect(m_searchResultsWidget, &SearchResultsWidget::nextButtonClicked, this, &HomePage::onNextButtonClicked);
}

void HomePage::onSearchTextChanged(const QString& text)
{
	Q_UNUSED(text);
	searchChanged = true;
}

void HomePage::onSearchClicked()
{
	if (!searchChanged) return;

	QString searchText = antInput->text().trimmed();
	if (searchText.isEmpty()) {
		m_searchResultsWidget->hide();
		return;
	}
	AntMessageManager::instance()->showMessage(AntMessage::Info, "链接解析中...");
	// 加载数据
	getVideoList(searchText);
	AntMessageManager::instance()->showMessage(AntMessage::Success, "解析成功！");
}

void HomePage::onNextButtonClicked()
{
	AntMessageManager::instance()->showMessage(AntMessage::Success, "数据解析中...");
	QList<int> selectedIndexes = m_searchResultsWidget->getSelectedIndexes();
	QList<VideoInfo> selectedVideoInfoList;
	for (size_t i = 0; i < videoInfoList.size(); i++)
	{
		if (selectedIndexes.contains(i))
			selectedVideoInfoList.append(videoInfoList[i]);
	}

	// 隐藏搜索结果组件
	m_searchResultsWidget->hide();

	// 清空搜索框
	antInput->clear();
	videoInfoList.clear();
	m_searchResultsWidget->clearAll();

	// 发出导航信号
	emit navigateToDownloadRequested(selectedVideoInfoList);
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

void HomePage::getVideoList(const QString& searchText)
{
	// 清空之前的结果
	videoInfoList.clear();
	m_searchResultsWidget->clearAll();
	//匹配网址前缀
	//QString modId = m_configModManager->findModForUrl(searchText);
	//if (modId.isEmpty())
	//{
	//	// 没有匹配的Mod，显示错误信息
	//	return;
	//}
	// 获取平台聚合服务
	auto platformService = m_appController->getPlatformService();
	if (!platformService) {
		LOG_ERROR("HomePage", "平台服务未初始化");
		AntMessageManager::instance()->showMessage(AntMessage::Error, "平台服务未初始化！");
		return;
	}

	// 检查是否有可用的平台
	auto availablePlatforms = platformService->getAvailablePlatforms();
	if (availablePlatforms.isEmpty()) {
		LOG_ERROR("HomePage", "没有可用的视频平台，请检查Mod配置");
		AntMessageManager::instance()->showMessage(AntMessage::Error, "无匹配的视频平台！");
		return;
	}

	LOG_INFO("HomePage", "开始搜索: %s", searchText.toUtf8().constData());

	// 启动搜索
	videoInfoList = platformService->getVideoInfo(searchText);
	if (!videoInfoList[0].isValid())
	{
		m_searchResultsWidget->hide();
		AntMessageManager::instance()->showMessage(AntMessage::Error, "视频链接不存在！");
		return;
	}
	QStringList Titles;
	QStringList Durations;
	QStringList Authors;
	for (int i = 0; i < videoInfoList.size(); i++)
	{
		if (videoInfoList[i].isValid())
		{
			// 添加搜索结果项
			Titles.append(videoInfoList[i].title);
			Durations.append(videoInfoList[i].duration);
			Authors.append(videoInfoList[i].author);
			m_searchResultsWidget->addSearchResultItem(Titles[i], Durations[i], Authors[i]);
		}
	}

	// 显示并定位搜索结果组件
	updateSearchResultsPosition();
	m_searchResultsWidget->show();
	m_searchResultsWidget->raise();

	m_searchResultsWidget->setFocus();

	searchChanged = false;
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