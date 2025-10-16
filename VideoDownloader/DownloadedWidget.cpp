#include "DownloadedWidget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDesktopServices>

DownloadedWidget::DownloadedWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: DownloadCardContainerWidget(downloadManager, parent)
{
	// 先设置无数据文本
	setNoDataText("暂无已下载任务");

	// 然后初始化UI
	initUI();
	updateTaskList();

	// 连接特定的信号
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

QList<DownloadTaskInfo> DownloadedWidget::getTaskList() const
{
	if (m_downloadManager) {
		auto tasks = m_downloadManager->getCompletedDownloads();
		QList<DownloadTaskInfo> filteredTasks;
		for (const auto& task : tasks) {
			if (task.status == Completed || task.status == Failed) {
				filteredTasks.append(task);
			}
		}
		return filteredTasks;
	}
	return QList<DownloadTaskInfo>();
}

void DownloadedWidget::setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo)
{
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
}

QString DownloadedWidget::getNoDataText() const
{
	return "暂无已下载任务";
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