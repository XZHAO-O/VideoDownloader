#include "DownloadCardContainerWidget.h"
#include <QScrollArea>

DownloadCardContainerWidget::DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
	, m_mainLayout(nullptr)
	, m_scrollArea(nullptr)
	, m_scrollWidget(nullptr)
	, m_scrollLayout(nullptr)
	, m_noDataWidget(nullptr)
	, m_noDataText("暂无任务") // 默认文本
{
	// 注意：不要在构造函数中调用任何虚函数
	// updateTaskList() 和 initUI() 将在派生类构造函数中调用
}

DownloadCardContainerWidget::~DownloadCardContainerWidget()
{
}

void DownloadCardContainerWidget::initUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

	// 创建滚动区域
	m_scrollArea = new AntScrollArea(AntScrollArea::ScrollVertical, this);
	m_scrollWidget = new QWidget(this);
	m_scrollLayout = new QVBoxLayout(m_scrollWidget);
	m_scrollLayout->setContentsMargins(16, 16, 16, 16);
	m_scrollLayout->setSpacing(12);
	m_scrollLayout->setAlignment(Qt::AlignTop);

	// 添加一个弹簧，让内容从顶部开始
	m_scrollLayout->addStretch();

	m_scrollArea->addWidget(m_scrollWidget);
	m_mainLayout->addWidget(m_scrollArea);

	// 创建暂无数据组件
	m_noDataWidget = new NoDataWidget(this);
	m_noDataWidget->setText(m_noDataText); // 使用成员变量而不是虚函数
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);
}

void DownloadCardContainerWidget::updateTaskList()
{
	// 清空当前卡片
	for (auto card : m_taskCards) {
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}
	m_taskCards.clear();

	// 获取任务列表
	if (m_downloadManager) {
		auto tasks = getTaskList(); // 现在这是安全的，因为对象已经完全构造
		for (const auto& task : tasks) {
			addTaskCard(task);
		}
	}

	updateVisibility();
}

// 其他方法保持不变...
void DownloadCardContainerWidget::addTaskCard(const DownloadTaskInfo& taskInfo)
{
	auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);
	auto card = new DownloadCard(model, this);

	// 在添加弹簧之前插入卡片
	m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, card);
	m_taskCards[taskInfo.taskId] = card;

	// 设置卡片连接
	setupCardConnections(card, taskInfo);

	updateVisibility();
}

void DownloadCardContainerWidget::removeTaskCard(const QString& taskId)
{
	if (m_taskCards.contains(taskId)) {
		auto card = m_taskCards.take(taskId);
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}

	updateVisibility();
}

void DownloadCardContainerWidget::addDownloadCard(DownloadCard* downloadCard)
{
	m_downloadCards.append(downloadCard);
	// 在添加弹簧之前插入卡片
	m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, downloadCard);

	// 连接信号
	connect(downloadCard, &DownloadCard::downloadClicked, this, [this]() {
		// 开始下载逻辑
		// VideoDownloadRequest request = ...;
		// m_downloadManager->downloadVideo(request);
		});

	// 连接删除信号
	connect(downloadCard, &DownloadCard::deleteClicked, this, [this, downloadCard]() {
		// 从布局中移除并删除卡片
		m_scrollLayout->removeWidget(downloadCard);
		m_downloadCards.removeOne(downloadCard);
		downloadCard->deleteLater();
		updateVisibility();
		});

	updateVisibility();
}

void DownloadCardContainerWidget::updateVisibility()
{
	bool hasTasks = !m_taskCards.isEmpty() || !m_downloadCards.isEmpty();
	m_scrollArea->setVisible(hasTasks);
	m_noDataWidget->setVisible(!hasTasks);
}

void DownloadCardContainerWidget::onDownloadAdded(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		if (!m_taskCards.contains(taskId)) {
			addTaskCard(taskInfo);
		}
	}
}

void DownloadCardContainerWidget::onDownloadRemoved(const QString& taskId)
{
	removeTaskCard(taskId);
}

void DownloadCardContainerWidget::onDownloadStatusChanged(const QString& taskId)
{
	// 基类默认实现，派生类可以重写
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		// 如果任务不在当前列表应该显示的状态中，移除卡片
		auto taskList = getTaskList();
		bool shouldShow = false;
		for (const auto& task : taskList) {
			if (task.taskId == taskId) {
				shouldShow = true;
				break;
			}
		}

		if (!shouldShow && m_taskCards.contains(taskId)) {
			removeTaskCard(taskId);
		}
		else if (shouldShow && !m_taskCards.contains(taskId)) {
			addTaskCard(taskInfo);
		}
	}
}