#include "DownloadPage.h"

#include <QtConcurrent\QtConcurrent>
#include <QFuture>


#include "AntScrollArea.h"
#include "SkeletonWidget.h"
#include "AntToggleButton.h"
#include "MaterialTabWidget.h"
#include "MaterialProgressBar.h"
#include "MaterialSpinner.h"
#include "AntSlider.h"
#include "NoDataWidget.h"
#include "AnimatedNumber.h"
#include "AntButton.h"
#include "NotificationManager.h"
#include "AntComboBox.h"
#include "TagWidget.h"
#include "CardWidget.h"
#include "FlowLayout.h"
#include "BadgeWidget.h"
#include "AntChatListView.h"
#include "DownloadCard.h"
#include "DownloadManager.h"
#include "ApplicationController.h"
#include "PlatformAggregatorService.h"
#include "ConfigVideoPlatform.h"
#include "DownloadCardContainerWidget.h"
#include "Instrumentor.h"

DownloadPage::DownloadPage(QSharedPointer<ApplicationController> applicationController, QWidget* parent)
	: QWidget(parent)
	, m_applicationController(applicationController)
	, m_downloadManager(applicationController->getDownloadManager())
{
	BENCHMARKING_FUNCTION();
	setObjectName("DownloadPage");
	setFocusPolicy(Qt::ClickFocus);  // 设置焦点策略 点击空白处可以获取焦点

	// 标签页
	tabWidget = new MaterialTabWidget(this);
	tabWidget->getLayout()->setContentsMargins(15, 0, 5, 0);

	// 滚动区域
	scrollArea1 = new AntScrollArea(AntScrollArea::ScrollVertical, this);
	QWidget* w1 = new QWidget(this);
	scrollArea1->addWidget(w1);

	downloadReadyWidget = new DownloadCardContainerWidget(m_downloadManager, ContainerState::DownloadReady, this);
	downloadingWidget = new DownloadCardContainerWidget(m_downloadManager, ContainerState::Downloading, this);
	downloadedWidget = new DownloadCardContainerWidget(m_downloadManager, ContainerState::Downloaded, this);

	// 连接任务状态改变信号
	connect(downloadReadyWidget, &DownloadCardContainerWidget::taskStateChanged,
		this, &DownloadPage::onTaskStateChanged);
	connect(downloadingWidget, &DownloadCardContainerWidget::taskStateChanged,
		this, &DownloadPage::onTaskStateChanged);
	connect(downloadedWidget, &DownloadCardContainerWidget::taskStateChanged,
		this, &DownloadPage::onTaskStateChanged);

	// 连接下载管理器信号
	connect(m_downloadManager.get(), &DownloadManager::downloadStarted,
		this, &DownloadPage::onDownloadManagerStarted);
	connect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
		this, &DownloadPage::onDownloadManagerCompleted);
	connect(m_downloadManager.get(), &DownloadManager::downloadProgress,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadProgress);
	connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadFailed);
	connect(m_downloadManager.get(), &DownloadManager::downloadPaused,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadStatusChanged);
	connect(m_downloadManager.get(), &DownloadManager::downloadResumed,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadStatusChanged);

	// 添加标签项
	tabWidget->addTab(downloadReadyWidget, "待下载");
	tabWidget->addTab(downloadingWidget, "下载中");
	tabWidget->addTab(downloadedWidget, "已下载");
	tabWidget->addTab(scrollArea1, "常用控件");

	// 主布局
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tabWidget);

	// 页面内容布局
	QVBoxLayout* pageLay = new QVBoxLayout(w1);
	pageLay->setSpacing(14);
	pageLay->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* row8Layout = new QHBoxLayout();
	row8Layout->setSpacing(1);
	row8Layout->setContentsMargins(0, 0, 0, 0);
	// 创建卡片模型
	auto cardModel = QSharedPointer<DownloadCardModel>::create();
	cardModel->setTitle("示例视频标题");
	cardModel->setDuration("12:34");
	cardModel->setPublishTime(QDateTime::currentDateTime().addDays(-2));
	cardModel->setPublisher("视频发布者");
	cardModel->setVideoSize(1024 * 1024 * 150); // 150MB
	cardModel->setAudioSize(1024 * 1024 * 20);  // 20MB
	cardModel->setProgress(50);
	cardModel->setState(DownloadCardState::Downloading);

	// 创建卡片
	auto downloadCard = new DownloadCard(cardModel, this);

	// 添加到布局中
	row8Layout->addWidget(downloadCard);

	// 添加到页面布局
	pageLay->addLayout(row8Layout);
	pageLay->addStretch();
}

DownloadPage::~DownloadPage()
{
}

void DownloadPage::getVideoPlayUrl(DownloadTaskInfo& taskInfo)
{
	BENCHMARKING_FUNCTION();
	auto platformService = m_applicationController->getPlatformService();
	auto videoPlatfrom = platformService->getPlatform(taskInfo.request.platformId);
	taskInfo.request.videoPlayUrl = videoPlatfrom->getVideoPlayUrl(taskInfo.streamRequest);
}

void DownloadPage::getVideoCover(DownloadTaskInfo& taskInfo)
{
	BENCHMARKING_FUNCTION();
	auto platformService = m_applicationController->getPlatformService();
	auto videoPlatfrom = platformService->getPlatform(taskInfo.request.platformId);
	videoPlatfrom->getVideoCover(taskInfo);
}

void DownloadPage::createDownloadCards(QList<VideoInfo>&& videoInfoList)
{
	BENCHMARKING_FUNCTION();
	downloadReadyWidget->showLoading();

	// 创建任务列表
	auto sharedTaskList = QSharedPointer<QList<DownloadTaskInfo>>::create();
	sharedTaskList->reserve(videoInfoList.size());

	// 保存移动后的列表到局部变量
	QList<VideoInfo> localVideoList = std::move(videoInfoList);

	auto* watcher = new QFutureWatcher<DownloadTaskInfo>(this);

	connect(watcher, &QFutureWatcher<DownloadTaskInfo>::resultReadyAt, this,
		[this, sharedTaskList, watcher](int index) {
			DownloadTaskInfo taskInfo = watcher->resultAt(index);
			sharedTaskList->append(std::move(taskInfo));
		});

	connect(watcher, &QFutureWatcher<DownloadTaskInfo>::finished, this,
		[this, watcher, sharedTaskList]() {
			downloadReadyWidget->addDownloadCards(std::move(*sharedTaskList));
			watcher->deleteLater();
		});

	// 使用局部变量（左值）而不是右值引用
	QFuture<DownloadTaskInfo> future = QtConcurrent::mapped(localVideoList,
		[this](const VideoInfo& videoInfo) {
			DownloadTaskInfo taskInfo;
			taskInfo.taskId = taskInfo.request.generateTaskId();
			taskInfo.request.platformId = videoInfo.platformId;
			taskInfo.streamRequest.extraParams.insert(videoInfo.extraParams);
			taskInfo.videoInfo = videoInfo;  // 这里不能移动，因为 videoInfo 是 const 引用

			getVideoPlayUrl(taskInfo);
			getVideoCover(taskInfo);
			taskInfo.request.outputPath = "E:/CProject/" + videoInfo.title + ".mp4";

			return taskInfo;
		});

	watcher->setFuture(future);
}

void DownloadPage::resizeEvent(QResizeEvent* event)
{
}

// 添加任务状态改变处理函数
void DownloadPage::onTaskStateChanged(const QString& taskId, ContainerState newState)
{
	BENCHMARKING_FUNCTION();
	DownloadTaskInfo taskInfo;
	ContainerState sourceState = ContainerState::DownloadReady;

	// 查找任务在哪个容器中
	if (!(taskInfo = downloadReadyWidget->getTaskInfo(taskId)).taskId.isEmpty()) {
		sourceState = ContainerState::DownloadReady;
	}
	else if (!(taskInfo = downloadingWidget->getTaskInfo(taskId)).taskId.isEmpty()) {
		sourceState = ContainerState::Downloading;
	}
	else {
		qDebug() << "Task not found:" << taskId;
		return;
	}

	// 如果源状态和目标状态相同，不处理
	if (sourceState == newState) {
		return;
	}

	// 从源容器移除任务
	switch (sourceState) {
	case ContainerState::DownloadReady:
		downloadReadyWidget->removeTask(taskId);
		break;
	case ContainerState::Downloading:
		downloadingWidget->removeTask(taskId);
		break;
	}

	// 更新任务状态以匹配目标容器状态
	switch (newState) {
	case ContainerState::Downloading:
		taskInfo.status = Downloading;
		break;
	case ContainerState::Downloaded:
		taskInfo.status = Completed;
		break;
	}

	// 添加到目标容器
	switch (newState) {
	case ContainerState::Downloading:
		downloadingWidget->transferTaskToThis(taskInfo);
		// 如果是转移到下载中，开始下载
		if (m_downloadManager && sourceState == ContainerState::DownloadReady) {
			// 只有从待下载转移时才调用addDownload
			m_downloadManager->addDownload(taskInfo);
		}
		break;
	case ContainerState::Downloaded:
		downloadedWidget->transferTaskToThis(taskInfo);
		break;
	}
}

// 处理下载管理器开始的信号
void DownloadPage::onDownloadManagerStarted(const QString& taskId)
{
	// 确保卡片状态正确更新
	downloadingWidget->onDownloadStarted(taskId);
}

// 处理下载管理器完成的信号
void DownloadPage::onDownloadManagerCompleted(const QString& taskId, const QString& filePath)
{
	BENCHMARKING_FUNCTION();
	// 更新任务信息中的文件路径
	DownloadTaskInfo taskInfo = downloadingWidget->getTaskInfo(taskId);
	if (!taskInfo.taskId.isEmpty()) {
		taskInfo.request.outputPath = filePath;
		taskInfo.status = Completed;
		// 转移到已下载容器
		onTaskStateChanged(taskId, ContainerState::Downloaded);
	}
}