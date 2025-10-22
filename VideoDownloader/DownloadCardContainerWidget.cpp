#include "DownloadCardContainerWidget.h"
#include <QScrollArea>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>

DownloadCardContainerWidget::DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager,
	ContainerState state,
	QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
	, m_containerState(state)
	, m_mainLayout(nullptr)
	, m_scrollArea(nullptr)
	, m_scrollWidget(nullptr)
	, m_scrollLayout(nullptr)
	, m_noDataWidget(nullptr)
	, m_currentSpeed(0)
{
	// 根据状态设置无数据文本
	switch (m_containerState) {
	case ContainerState::DownloadReady:
		m_noDataText = "暂无待下载任务";
		break;
	case ContainerState::Downloading:
		m_noDataText = "暂无下载任务";
		break;
	case ContainerState::Downloaded:
		m_noDataText = "暂无已下载任务";
		break;
	}

	initUI();
	updateTaskList();

	// 连接信号
	if (m_downloadManager) {
		connect(m_downloadManager.get(), &DownloadManager::downloadAdded,
			this, &DownloadCardContainerWidget::onDownloadAdded);
		connect(m_downloadManager.get(), &DownloadManager::downloadStatusChanged,
			this, &DownloadCardContainerWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
			this, &DownloadCardContainerWidget::onDownloadCompleted);
		connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
			this, &DownloadCardContainerWidget::onDownloadFailed);
		connect(m_downloadManager.get(), &DownloadManager::downloadProgress,
			this, &DownloadCardContainerWidget::onDownloadProgress);
		connect(m_downloadManager.get(), &DownloadManager::downloadSpeedUpdated, this, &DownloadCardContainerWidget::onDownloadSpeedUpdated);
	}
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
	m_noDataWidget->setText(m_noDataText);
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
		auto tasks = getTaskList();
		for (const auto& task : tasks) {
			addTaskCard(task);
		}
	}

	updateVisibility();
}

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

QList<DownloadTaskInfo> DownloadCardContainerWidget::getTaskList() const
{
	if (!m_downloadManager) {
		return QList<DownloadTaskInfo>();
	}

	switch (m_containerState) {
	case ContainerState::DownloadReady:
		return m_downloadManager->getQueuedDownloads();

	case ContainerState::Downloading: {
		auto tasks = m_downloadManager->getActiveDownloads();
		QList<DownloadTaskInfo> filteredTasks;
		for (const auto& task : tasks) {
			if (task.status == Downloading || task.status == Paused) {
				filteredTasks.append(task);
			}
		}
		return filteredTasks;
	}

	case ContainerState::Downloaded: {
		auto tasks = m_downloadManager->getCompletedDownloads();
		QList<DownloadTaskInfo> filteredTasks;
		for (const auto& task : tasks) {
			if (task.status == Completed || task.status == Failed) {
				filteredTasks.append(task);
			}
		}
		return filteredTasks;
	}
	}

	return QList<DownloadTaskInfo>();
}

void DownloadCardContainerWidget::setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo)
{
	switch (m_containerState) {
	case ContainerState::DownloadReady:
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
		break;

	case ContainerState::Downloading:
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
		break;

	case ContainerState::Downloaded:
		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			// 打开文件所在文件夹
			QFileInfo fileInfo(taskInfo.request.outputPath);
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo]() {
			if (m_downloadManager) {
				// 从已完成列表中移除
				removeTaskCard(taskInfo.taskId);
			}
			});
		break;
	}
}

QString DownloadCardContainerWidget::getNoDataText() const
{
	return m_noDataText;
}

void DownloadCardContainerWidget::downloadVideo(const QUrl& url)
{
	// 创建目录
	QString savePath = "E:/CProject";
	QDir dir(savePath);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	qDebug() << url;
	QString fileName = QFileInfo(url.path()).fileName();
	QString filePath = savePath + "/" + fileName;
	// 打开文件
	QFile* m_file = new QFile(filePath, this);
	if (!m_file->open(QIODevice::WriteOnly)) {
		delete m_file;
		m_file = nullptr;
		return;
	}

	// 创建网络请求
	QNetworkRequest request;
	request.setUrl(url);

	request.setRawHeader("User-Agent",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
	request.setRawHeader("Referer", "https://www.bilibili.com");
	request.setRawHeader("Origin", "https://www.bilibili.com");

	QNetworkAccessManager* m_manager = new QNetworkAccessManager(this);

	// 开始下载
	QNetworkReply* m_reply = m_manager->get(request);
	// 连接信号处理下载数据
	connect(m_reply, &QNetworkReply::readyRead, this, [this, m_reply, m_file]() {
		// 将可用数据写入文件
		if (m_reply->bytesAvailable() > 0) {
			m_file->write(m_reply->readAll());
		}
		});

	// 处理下载完成
	connect(m_reply, &QNetworkReply::finished, this, [this, m_reply, m_file, m_manager]() {
		// 写入剩余数据
		if (m_reply->bytesAvailable() > 0) {
			m_file->write(m_reply->readAll());
		}

		m_file->close();

		if (m_reply->error() == QNetworkReply::NoError) {
			qDebug() << "下载完成";
		}
		else {
			qDebug() << "下载失败:" << m_reply->errorString();
			// 删除不完整的文件
			m_file->remove();
		}

		// 清理资源
		m_reply->deleteLater();
		m_file->deleteLater();
		m_manager->deleteLater();
		});

	// 下载进度
	connect(m_reply, &QNetworkReply::downloadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal) {
		if (bytesTotal > 0) {
			double percent = (double(bytesReceived) / double(bytesTotal)) * 100.0;
			qDebug() << "下载进度:" << bytesReceived << "/" << bytesTotal
				<< "(" << QString::number(percent, 'f', 1) << "%)";
		}
		});
}

void DownloadCardContainerWidget::addDownloadCard(DownloadTaskInfo downloadTaskInfo, DownloadCard* downloadCard)
{
	m_downloadTasks.append(downloadTaskInfo);
	m_downloadCards.append(downloadCard);
	// 在添加弹簧之前插入卡片
	m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, downloadCard);

	// 连接信号
	connect(downloadCard, &DownloadCard::downloadClicked, this, [this, downloadTaskInfo]() {
		downloadVideo(downloadTaskInfo.request.videoPlayUrl);
		});

	// 连接删除信号
	connect(downloadCard, &DownloadCard::deleteClicked, this, [this, downloadCard, downloadTaskInfo]() {
		// 断开所有连接
		downloadCard->disconnect();
		// 从布局中移除并删除卡片
		m_scrollLayout->removeWidget(downloadCard);
		m_downloadCards.removeOne(downloadCard);
		m_downloadTasks.removeOne(downloadTaskInfo);
		delete downloadCard;
		updateVisibility();
		});

	if (m_downloadCards.size() == 1)
	{
		updateVisibility();
	}
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
			// 检查任务是否应该显示在当前容器中
			auto taskList = getTaskList();
			for (const auto& task : taskList) {
				if (task.taskId == taskId) {
					addTaskCard(taskInfo);
					break;
				}
			}
		}
	}
}

void DownloadCardContainerWidget::onDownloadRemoved(const QString& taskId)
{
	removeTaskCard(taskId);
}

void DownloadCardContainerWidget::onDownloadStatusChanged(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);

		// 检查任务是否应该显示在当前容器中
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
		else if (shouldShow && m_taskCards.contains(taskId)) {
			// 更新现有卡片状态
			auto card = m_taskCards[taskId];
			auto model = card->model();
			if (model) {
				switch (taskInfo.status) {
				case Queued:
					model->setState(DownloadCardState::Pending);
					break;
				case Downloading:
					model->setState(DownloadCardState::Downloading);
					break;
				case Completed:
					model->setState(DownloadCardState::Downloaded);
					break;
				case Failed:
					model->setState(DownloadCardState::Error);
					break;
				default:
					break;
				}
			}
		}
	}
}

void DownloadCardContainerWidget::onDownloadCompleted(const QString& taskId, const QString& filePath)
{
	onDownloadStatusChanged(taskId);
}

void DownloadCardContainerWidget::onDownloadFailed(const QString& taskId, const QString& error)
{
	onDownloadStatusChanged(taskId);
}

void DownloadCardContainerWidget::onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	updateTaskProgress(taskId, downloaded, total);
}

void DownloadCardContainerWidget::onDownloadSpeedUpdated(qint64 bytesPerSecond)
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

void DownloadCardContainerWidget::updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total)
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

void DownloadCardContainerWidget::setState(ContainerState state)
{
	if (m_containerState != state) {
		m_containerState = state;

		// 更新无数据文本
		switch (m_containerState) {
		case ContainerState::DownloadReady:
			m_noDataText = "暂无待下载任务";
			break;
		case ContainerState::Downloading:
			m_noDataText = "暂无下载任务";
			break;
		case ContainerState::Downloaded:
			m_noDataText = "暂无已下载任务";
			break;
		}

		if (m_noDataWidget) {
			m_noDataWidget->setText(m_noDataText);
		}

		// 更新任务列表
		updateTaskList();
	}
}