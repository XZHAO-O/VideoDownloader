#include "HomePage.h"

#include <QSet>
#include <QGridLayout>

#include "AntButton.h"
#include "AntInput.h"
#include "AntMessageManager.h"
#include "PlatformAggregatorService.h"
#include "ConfigModManager.h"
#include "SearchResultsWidget.h"
#include "LogSystem.h"

HomePage::HomePage(QSharedPointer<PlatformAggregatorService> platformService, QSharedPointer<ConfigModManager> configModManager, QWidget* parent)
	: QWidget(parent)
	, m_platformService(platformService)
	, m_configModManager(configModManager)
	, m_availablePlatforms(platformService->getAvailablePlatforms())
	, antInput(nullptr)
	, m_searchResultsWidget(nullptr)
{
	setObjectName("HomePage");

	setupUI();
	setupConnections();
}

HomePage::~HomePage()
{
}

void HomePage::setupUI()
{
	QStringList listItems = {};
	antInput = new AntInput(300, listItems, this);
	antInput->setFixedSize(400, 50);
	antInput->setPlaceholderText("搜索视频内容...");

	// 创建搜索结果组件
	m_searchResultsWidget = new SearchResultsWidget(this);
	m_searchResultsWidget->hide();

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
	connect(m_configModManager.get(), &ConfigModManager::modsChanged, this, &HomePage::availablePlatformsChanged);
}

void HomePage::availablePlatformsChanged()
{
	m_availablePlatforms = m_platformService->getAvailablePlatforms();
}

void HomePage::onSearchTextChanged(const QString& text)
{
	Q_UNUSED(text);
	searchChanged = true;
}

void HomePage::onSearchClicked()
{
	if (!searchChanged) return;

	searchChanged = false;

	const QString searchText = antInput->text().trimmed();
	if (searchText.isEmpty())
	{
		m_searchResultsWidget->hide();
		clearSearchData();
		return;
	}

	// 加载数据
	getVideoList(searchText);
}

void HomePage::onNextButtonClicked()
{
	AntMessageManager::instance()->showMessage(AntMessage::Info, "数据解析中...");

	const QList<int>& selectedIndexes = m_searchResultsWidget->getSelectedIndexes();

	if (selectedIndexes.isEmpty())
		return;

	// 使用QSet提高查找性能
	const QSet<int> selectedSet(selectedIndexes.begin(), selectedIndexes.end());
	QList<VideoInfo> selectedVideoInfoList;
	selectedVideoInfoList.reserve(selectedIndexes.size()); // 预分配内存

	for (int i = 0; i < videoInfoList.size(); ++i)
	{
		if (selectedSet.contains(i))
		{
			selectedVideoInfoList.append(videoInfoList[i]);
		}
	}

	// 隐藏搜索结果组件
	m_searchResultsWidget->hide();

	// 清空搜索数据（包括输入框）
	clearSearchData();

	// 发出导航信号
	emit navigateToDownloadRequested(std::move(selectedVideoInfoList));
}

void HomePage::updateSearchResultsPosition()
{
	if (!m_searchResultsWidget || !antInput) return;

	// 获取搜索框的全局位置
	const QPoint inputGlobalPos = antInput->mapToGlobal(QPoint(0, 0));
	// 转换为相对于HomePage的位置
	const QPoint inputLocalPos = mapFromGlobal(inputGlobalPos);

	// 设置搜索结果组件的位置和大小
	const int containerWidth = antInput->width();
	const int containerHeight = static_cast<int>(height() * 0.6);

	m_searchResultsWidget->setFixedSize(containerWidth, containerHeight);
	m_searchResultsWidget->move(inputLocalPos.x(), inputLocalPos.y() + antInput->height() + 8);
}

void HomePage::getVideoList(const QString& searchText)
{
	// 清空之前的结果（但不包括输入框）
	videoInfoList.clear();
	m_searchResultsWidget->clearAll();

	// 检查是否有可用的平台
	if (m_availablePlatforms.isEmpty())
	{
		LOG_ERROR("HomePage", "没有可用的视频平台");
		AntMessageManager::instance()->showMessage(AntMessage::Error, "无匹配的视频平台！");
		return;
	}

	AntMessageManager::instance()->showMessage(AntMessage::Info, "链接解析中...");
	LOG_INFO("HomePage", "开始解析视频链接: %s", searchText.toUtf8().constData());

	// 启动搜索(搜索请求超时处理未添加)
	videoInfoList = m_platformService->getVideoInfo(searchText);

	if (videoInfoList.isEmpty() || !videoInfoList.first().isValid())
	{
		m_searchResultsWidget->hide();
		AntMessageManager::instance()->showMessage(AntMessage::Error, "视频链接不存在！");
		return;
	}

	// 处理视频列表
	processVideoList(videoInfoList);

	AntMessageManager::instance()->showMessage(AntMessage::Success, "解析成功！");
}

void HomePage::processVideoList(const QList<VideoInfo>& videos)
{
	// 批量添加搜索结果，减少UI更新次数
	m_searchResultsWidget->clearAll();
	m_searchResultsWidget->addSearchResultItems(videos); // 使用批量添加

	// 显示并定位搜索结果组件
	updateSearchResultsPosition();
	m_searchResultsWidget->show();
	m_searchResultsWidget->raise();
	m_searchResultsWidget->setFocus();
}

void HomePage::clearSearchData()
{
	videoInfoList.clear();
	m_searchResultsWidget->clearAll();
	antInput->clear();
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