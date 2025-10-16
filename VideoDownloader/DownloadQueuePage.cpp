#include "DownloadQueuePage.h"
#include <QScrollArea>

DownloadQueuePage::DownloadQueuePage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: DownloadCardContainerWidget(downloadManager, parent)
{
	// 先设置无数据文本
	setNoDataText("暂无待下载任务");

	// 然后初始化UI
	initUI();
	updateTaskList();

	// 连接特定的信号
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

QList<DownloadTaskInfo> DownloadQueuePage::getTaskList() const
{
	if (m_downloadManager) {
		return m_downloadManager->getQueuedDownloads();
	}
	return QList<DownloadTaskInfo>();
}

void DownloadQueuePage::setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo)
{
	connect(card, &DownloadCard::downloadClicked, this, [this, taskInfo]() {
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