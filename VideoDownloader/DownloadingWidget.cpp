#include "DownloadingWidget.h"
#include <QScrollArea>

DownloadingWidget::DownloadingWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
	, m_currentSpeed(0)
{
	initUI();
	updateTaskList();

	// 连接信号 - 修复这里的信号名称
	if (m_downloadManager) {
		connect(m_downloadManager.get(), &DownloadManager::downloadProgress,
			this, &DownloadingWidget::onDownloadProgress);
		connect(m_downloadManager.get(), &DownloadManager::downloadStatusChanged,
			this, &DownloadingWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadSpeedUpdated,
			this, &DownloadingWidget::onDownloadSpeedUpdated);

		// 添加其他必要的状态变化信号连接
		connect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
			this, &DownloadingWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
			this, &DownloadingWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadPaused,
			this, &DownloadingWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadResumed,
			this, &DownloadingWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadCancelled,
			this, &DownloadingWidget::onDownloadStatusChanged);
	}
}

DownloadingWidget::~DownloadingWidget()
{
}

void DownloadingWidget::initUI()
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
	m_noDataWidget->setText("暂无下载任务");
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);
}

void DownloadingWidget::updateTaskList()
{
	// 清空当前卡片
	for (auto card : m_taskCards) {
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}
	m_taskCards.clear();

	// 获取下载中任务列表
	if (m_downloadManager) {
		auto tasks = m_downloadManager->getActiveDownloads();

		for (const auto& task : tasks) {
			if (task.status == Downloading || task.status == Paused) {
				addTaskCard(task);
			}
		}
	}

	// 显示/隐藏暂无数据提示
	bool hasTasks = !m_taskCards.isEmpty();
	m_scrollArea->setVisible(hasTasks);
	m_noDataWidget->setVisible(!hasTasks);
}

void DownloadingWidget::addTaskCard(const DownloadTaskInfo& taskInfo)
{
	auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);
	auto card = new DownloadCard(model, this);

	m_scrollLayout->addWidget(card);
	m_taskCards[taskInfo.taskId] = card;

	// 连接卡片信号
	connect(card, &DownloadCard::pauseClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			m_downloadManager->pauseDownload(taskInfo.taskId);
		}
		});

	connect(card, &DownloadCard::resumeClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			m_downloadManager->resumeDownload(taskInfo.taskId);
		}
		});

	connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo]() {
		if (m_downloadManager) {
			m_downloadManager->cancelDownload(taskInfo.taskId);
		}
		});

	// 显示滚动区域，隐藏暂无数据
	m_scrollArea->setVisible(true);
	m_noDataWidget->setVisible(false);
}

void DownloadingWidget::removeTaskCard(const QString& taskId)
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

void DownloadingWidget::updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	if (m_taskCards.contains(taskId)) {
		auto card = m_taskCards[taskId];
		auto model = card->model();
		if (model) {
			int progress = total > 0 ? static_cast<int>((downloaded * 100) / total) : 0;
			model->setProgress(progress);
			model->setDownloadSpeed(m_currentSpeed);
		}
	}
}

void DownloadingWidget::onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	updateTaskProgress(taskId, downloaded, total);
}

void DownloadingWidget::onDownloadStatusChanged(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);

		if (taskInfo.status == Downloading || taskInfo.status == Paused) {
			// 任务开始下载或暂停，添加到当前页面
			if (!m_taskCards.contains(taskId)) {
				addTaskCard(taskInfo);
			}
			else {
				// 更新现有卡片状态
				if (m_taskCards.contains(taskId)) {
					auto card = m_taskCards[taskId];
					auto model = card->model();
					if (model) {
						model->setState(taskInfo.status == Downloading ?
							DownloadCardState::Downloading : DownloadCardState::Pending);
					}
				}
			}
		}
		else {
			// 任务完成或失败，从当前页面移除
			removeTaskCard(taskId);
		}
	}
}

void DownloadingWidget::onDownloadSpeedUpdated(qint64 bytesPerSecond)
{
	m_currentSpeed = bytesPerSecond;

	// 更新所有卡片的下载速度
	for (auto card : m_taskCards) {
		auto model = card->model();
		if (model && model->state() == DownloadCardState::Downloading) {
			model->setDownloadSpeed(bytesPerSecond);
		}
	}
}