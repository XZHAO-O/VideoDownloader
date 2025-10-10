#include "DownloadedWidget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDesktopServices>

DownloadedWidget::DownloadedWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
{
	initUI();
	updateTaskList();

	// 连接信号
	if (m_downloadManager) {
		connect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
			this, &DownloadedWidget::onDownloadCompleted);
		connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
			this, &DownloadedWidget::onDownloadFailed);
	}
}

DownloadedWidget::~DownloadedWidget()
{
}

void DownloadedWidget::initUI()
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
	m_noDataWidget->setText("暂无已下载任务");
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);
}

void DownloadedWidget::updateTaskList()
{
	// 清空当前卡片
	for (auto card : m_taskCards) {
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}
	m_taskCards.clear();

	// 获取已下载任务列表
	if (m_downloadManager) {
		auto tasks = m_downloadManager->getCompletedDownloads();

		for (const auto& task : tasks) {
			if (task.status == Completed || task.status == Failed) {
				addTaskCard(task);
			}
		}
	}

	// 显示/隐藏暂无数据提示
	bool hasTasks = !m_taskCards.isEmpty();
	m_scrollArea->setVisible(hasTasks);
	m_noDataWidget->setVisible(!hasTasks);
}

void DownloadedWidget::addTaskCard(const DownloadTaskInfo& taskInfo)
{
	auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);

	// 设置正确的状态
	if (taskInfo.status == Completed) {
		model->setState(DownloadCardState::Downloaded);
	}
	else if (taskInfo.status == Failed) {
		model->setState(DownloadCardState::Error);
	}

	auto card = new DownloadCard(model, this);

	m_scrollLayout->addWidget(card);
	m_taskCards[taskInfo.taskId] = card;

	// 连接卡片信号
	connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
		// 打开文件所在文件夹
		QFileInfo fileInfo(taskInfo.request.outputPath);
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
		});

	connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			// 从已完成列表中移除
			// 注意：这里需要实现从已完成列表中移除的方法
			removeTaskCard(taskInfo.taskId);
		}
		});

	// 显示滚动区域，隐藏暂无数据
	m_scrollArea->setVisible(true);
	m_noDataWidget->setVisible(false);
}

void DownloadedWidget::removeTaskCard(const QString& taskId)
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

void DownloadedWidget::onDownloadCompleted(const QString& taskId, const QString& filePath)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		if (taskInfo.status == Completed) {
			addTaskCard(taskInfo);
		}
	}
}

void DownloadedWidget::onDownloadFailed(const QString& taskId, const QString& error)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		if (taskInfo.status == Failed) {
			addTaskCard(taskInfo);
		}
	}
}