#include "DownloadQueuePage.h"
#include <QScrollArea>

DownloadQueuePage::DownloadQueuePage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
{
	initUI();
	updateTaskList();

	// 连接信号
	if (m_downloadManager) {
		connect(m_downloadManager.get(), &DownloadManager::downloadAdded,
			this, &DownloadQueuePage::onDownloadAdded);
		connect(m_downloadManager.get(), &DownloadManager::downloadProgress,
			this, &DownloadQueuePage::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
			this, &DownloadQueuePage::onDownloadRemoved);
		connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
			this, &DownloadQueuePage::onDownloadRemoved);
		connect(m_downloadManager.get(), &DownloadManager::downloadCancelled,
			this, &DownloadQueuePage::onDownloadRemoved);
	}
}

DownloadQueuePage::~DownloadQueuePage()
{
}

void DownloadQueuePage::initUI()
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

	m_scrollArea->addWidget(m_scrollWidget);
	m_mainLayout->addWidget(m_scrollArea);

	// 创建暂无数据组件
	m_noDataWidget = new NoDataWidget(this);
	m_noDataWidget->setText("暂无待下载任务");
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);
}

void DownloadQueuePage::updateTaskList()
{
	// 清空当前卡片
	for (auto card : m_taskCards) {
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}
	m_taskCards.clear();

	// 获取待下载任务列表
	if (m_downloadManager) {
		auto tasks = m_downloadManager->getQueuedDownloads();

		for (const auto& task : tasks) {
			addTaskCard(task);
		}
	}

	// 显示/隐藏暂无数据提示
	bool hasTasks = !m_taskCards.isEmpty();
	m_scrollArea->setVisible(hasTasks);
	m_noDataWidget->setVisible(!hasTasks);
}

void DownloadQueuePage::addTaskCard(const DownloadTaskInfo& taskInfo)
{
	auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);
	auto card = new DownloadCard(model, this);

	m_scrollLayout->addWidget(card);
	m_taskCards[taskInfo.taskId] = card;

	// 连接卡片信号
	connect(card, &DownloadCard::downloadClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			// 开始下载任务
			m_downloadManager->resumeDownload(taskInfo.taskId);
		}
		});

	connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			// 取消任务
			m_downloadManager->cancelDownload(taskInfo.taskId);
		}
		});

	// 显示滚动区域，隐藏暂无数据
	m_scrollArea->setVisible(true);
	m_noDataWidget->setVisible(false);
}

void DownloadQueuePage::removeTaskCard(const QString& taskId)
{
	if (m_taskCards.contains(taskId)) {
		auto card = m_taskCards.take(taskId);
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}

	// 检查是否还有任务
	if (m_taskCards.isEmpty()) {
		m_scrollArea->setVisible(false);
		m_noDataWidget->setVisible(true);
	}
}

void DownloadQueuePage::onDownloadAdded(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		if (taskInfo.status == Queued) {
			addTaskCard(taskInfo);
		}
	}
}

void DownloadQueuePage::onDownloadRemoved(const QString& taskId)
{
	removeTaskCard(taskId);
}

void DownloadQueuePage::onDownloadStatusChanged(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		// 如果任务状态不再是待下载，从当前页面移除
		if (taskInfo.status != Queued && m_taskCards.contains(taskId)) {
			removeTaskCard(taskId);
		}
	}
}