#include "DownloadingWidget.h"
#include <QScrollArea>

DownloadingWidget::DownloadingWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: DownloadCardContainerWidget(downloadManager, parent)
	, m_currentSpeed(0)
{
	// 先设置无数据文本
	setNoDataText("暂无下载任务");

	// 然后初始化UI
	initUI();
	updateTaskList();

	// 连接特定的信号
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

QList<DownloadTaskInfo> DownloadingWidget::getTaskList() const
{
	if (m_downloadManager) {
		auto tasks = m_downloadManager->getActiveDownloads();
		QList<DownloadTaskInfo> filteredTasks;
		for (const auto& task : tasks) {
			if (task.status == Downloading || task.status == Paused) {
				filteredTasks.append(task);
			}
		}
		return filteredTasks;
	}
	return QList<DownloadTaskInfo>();
}

void DownloadingWidget::setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo)
{
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
}

QString DownloadingWidget::getNoDataText() const
{
	return "暂无下载任务";
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