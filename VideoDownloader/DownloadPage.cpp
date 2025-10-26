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

DownloadPage::DownloadPage(QSharedPointer<ApplicationController> applicationController, QWidget* parent)
	: QWidget(parent)
	, m_applicationController(applicationController)
	, m_downloadManager(applicationController->getDownloadManager())
{
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
	if (m_downloadManager) {
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
	}

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

	QHBoxLayout* carouselLayout = new QHBoxLayout();
	carouselLayout->setSpacing(0);
	carouselLayout->setContentsMargins(0, 0, 0, 0);

	// 添加标题 + 开关按钮
	QHBoxLayout* row1Layout = new QHBoxLayout();
	row1Layout->setSpacing(20); // label 和按钮之间的间距

	QHBoxLayout* row2Layout = new QHBoxLayout();
	row2Layout->setSpacing(10);

	QStringList nameList;
	QList<QLabel*>labelList;
	nameList << "开关按钮" << "进度条" << "水平滑动条" << "统计数值";
	for (auto& str : nameList)
	{
		QLabel* nameLabel = new QLabel(str, this);
		nameLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
		nameLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		labelList.append(nameLabel);
	}
	// 开关按钮
	AntToggleButton* toggleBtn = new AntToggleButton(QSize(57, 26), this);
	toggleBtn->setShowText(true);

	// 进度条
	MaterialProgressBar* progress = new MaterialProgressBar(this);
	progress->setFixedHeight(10);
	// 启动测试模式
	progress->startTestPattern();
	// 圆形进度条
	MaterialSpinner* spinner = new MaterialSpinner(QSize(40, 40), 4, DesignSystem::instance()->primaryColor(), this);

	// 水平滑动条
	AntSlider* slider = new AntSlider(0, 100, 30, this);
	slider->setFixedWidth(230);

	// 统计数值
	AnimatedNumber* animNum = new AnimatedNumber(this);
	animNum->setTextWidth(120);
	animNum->setFontSzie(16);
	animNum->animateTo(12345);

	// 骨架屏
	AntButton* skeletonDescBtn = new AntButton("骨架屏启动", 12, w1);
	skeletonDescBtn->setFixedSize(120, 50);
	QHBoxLayout* row3Layout = new QHBoxLayout();
	row3Layout->setSpacing(8);
	row3Layout->setContentsMargins(0, 0, 0, 16);

	QList<QIcon> icons = {
		QIcon(":/Imgs/undraw_book-lover_f1dq.svg"),
		QIcon(":/Imgs/undraw_developer-avatar_f6ac.svg"),
		QIcon(":/Imgs/undraw_loving-it_hspq.svg")
	};

	QList<QLabel*> iconLabels;

	for (const QIcon& icon : icons)
	{
		SkeletonWidget* skeleton = new SkeletonWidget(8, this);
		skeleton->setFixedSize(300, 200);
		skeleton->stopSkeleton(); // 初始状态下停止骨架屏
		skeleton->hide();         // 初始隐藏骨架屏
		row3Layout->addWidget(skeleton);
		skeletons.append(skeleton);  // 保存骨架指针

		QLabel* iconLabel = new QLabel(this);
		iconLabel->setAlignment(Qt::AlignCenter);
		QPixmap pixmap = icon.pixmap(350, 250);
		if (!pixmap.isNull()) {
			iconLabel->setPixmap(pixmap);
			iconLabel->show();  // 内容已加载好，直接显示真实图标
		}
		iconLabels.append(iconLabel);
		row3Layout->addWidget(iconLabel);
	}

	row3Layout->addStretch();

	connect(skeletonDescBtn, &AntButton::clicked, this, [this, skeletonDescBtn, iconLabels]()
		{
			skeletonDescBtn->setEnabled(false);  // 禁用按钮，防止重复点击
			// 启动骨架动画
			for (int i = 0; i < iconLabels.size(); ++i)
			{
				iconLabels[i]->hide();  // 隐藏真实内容
				skeletons[i]->show();   // 显示骨架屏
				skeletons[i]->startSkeleton(); // 启动骨架动画
			}

			// 模拟数据加载过程（实际项目中这里应该是真实的异步加载）
			QTimer::singleShot(1500, this, [=]()
				{
					for (int i = 0; i < skeletons.size(); ++i)
					{
						skeletons[i]->stopSkeleton();   // 停止并隐藏骨架
					}

					for (int i = 0; i < iconLabels.size(); ++i)
					{
						iconLabels[i]->show();
					}
					skeletonDescBtn->setEnabled(true);
				});
		});

	// 任务通知
	QHBoxLayout* row4Layout = new QHBoxLayout();
	row4Layout->setSpacing(18);
	row4Layout->setContentsMargins(0, 0, 0, 0);

	AntButton* taskBtn = new AntButton("任务通知", 12, w1);
	taskBtn->setFixedSize(120, 50);
	connect(taskBtn, &AntButton::clicked, this, [this]()
		{
			NotificationManager::instance()->showNotification("任务通知");
		});

	QHBoxLayout* row5Layout = new QHBoxLayout();
	row5Layout->setSpacing(18);
	row5Layout->setContentsMargins(0, 0, 0, 0);

	// 单层级下拉框
	QLabel* comboLabel1 = new QLabel("下拉框", this);
	// 多层级下拉框
	QLabel* comboLabel2 = new QLabel("多层级下拉框", this);
	QStringList topItems2 = { "水果", "蔬菜", "饮料" };
	AntComboBox* combo2 = new AntComboBox("请选择", topItems2, this);
	combo2->setFixedSize(185, 48);

	// 让下拉框遮罩跟随页面大小变化
	connect(this, &DownloadPage::resized, this, [=](int w, int h)
		{
			combo2->getMask()->resize(w, h);
		});

	// 下拉框子菜单定位
	connect(this, &DownloadPage::windowMoved, this, [combo2](QPoint globalPos)
		{
			auto movePopups = [globalPos, combo2](AntComboBox* combo)
				{
					for (PopupViewController* popup : combo->popupViewList())
					{
						// 多层下级拉框
						if (combo == combo2)
						{
							combo->popupViewList()[0]->follow(combo2);
							//combo2->popupViewList()[1]->follow(combo->popupViewList()[0], PopupViewController::TopRight);
						}
					}
				};
			movePopups(combo2);
		});

	// 标签
	QHBoxLayout* row6Layout = new QHBoxLayout();
	row6Layout->setSpacing(10);
	row6Layout->setContentsMargins(0, 22, 0, 22);

	QLabel* tagLabel = new QLabel("标签", this);

	// 初始化配置信息列表
	QList<TagWidget::TagInfo> tagList1 = {
		{"bilibili", ":/Imgs/bilibili.svg", QColor(251, 114, 153)},
		{"x",        ":/Imgs/x.svg",        QColor(0, 0, 0)},
		{"wechat",   ":/Imgs/WeChat.svg",   QColor(7, 193, 96)},
		{"taobao",   ":/Imgs/taobao.svg",   QColor(255, 140, 0)},
	};

	QList<TagWidget::TagInfo> tagList2 = {
		{"bilibili", "", QColor(251, 114, 153) },
		{"x",        "", QColor(0, 0, 0)},
		{"wechat",   "", QColor(7, 193, 96)},
		{"taobao",   "", QColor(255, 140, 0)},
	};

	row6Layout->addWidget(tagLabel);

	for (const TagWidget::TagInfo& info : tagList1)
	{
		TagWidget* tag = new TagWidget(info.name, 11.5, info.color, this, true, info.svgPath);
		row6Layout->addWidget(tag);
	}

	for (const TagWidget::TagInfo& info : tagList2)
	{
		TagWidget* tag = new TagWidget(info.name, 11.5, info.color, this, false);
		row6Layout->addWidget(tag);
	}

	// 卡片控件
	QHBoxLayout* row7Layout = new QHBoxLayout();
	row7Layout->setSpacing(10);
	row7Layout->setContentsMargins(0, 22, 0, 22);

	CardWidget* card = new CardWidget("上次登录：xxxx-xx-xx", "总运行时间：xxx小时", this);
	card->setImageFile(":/Imgs/gpt.jpg");
	card->setFixedSize(320, 200);
	QLabel* cardLabel = new QLabel("卡片", this);

	// 徽章
	// 创建徽章和标签列表
	QList<BadgeWidget*> badgeWidgets = {
		new BadgeWidget(10, 10, this),  // 圆形徽章
		new BadgeWidget(42, 22, this),  // 圆角矩形徽章
		new BadgeWidget(10, 10, this),  // 成功徽章
		new BadgeWidget(10, 10, this),  // 错误徽章
		new BadgeWidget(10, 10, this),  // 运行中徽章
		new BadgeWidget(10, 10, this),  // 警告徽章
		new BadgeWidget(10, 10, this)   // 默认徽章
	};

	// 设置徽章的状态
	badgeWidgets[0]->setBadge(true, BadgeWidget::None);
	badgeWidgets[1]->setBadge(false, BadgeWidget::None, 100, 99);
	badgeWidgets[2]->setBadge(true, BadgeWidget::Success);
	badgeWidgets[3]->setBadge(true, BadgeWidget::Error);
	badgeWidgets[4]->setBadge(true, BadgeWidget::Running);
	badgeWidgets[5]->setBadge(true, BadgeWidget::Warning);
	badgeWidgets[6]->setBadge(true, BadgeWidget::Default);

	// 创建标签
	QList<QLabel*> badgeLabelList = {
		new QLabel("圆形徽章", this),
		new QLabel("圆角矩形徽章", this),
		new QLabel("成功", this),
		new QLabel("错误", this),
		new QLabel("运行中", this),
		new QLabel("警告", this),
		new QLabel("默认", this)
	};

	// 第九行布局
	QHBoxLayout* row9Layout = new QHBoxLayout();
	row9Layout->setContentsMargins(0, 0, 0, 20);
	row9Layout->setSpacing(10);

	// 循环添加标签和徽章
	for (int i = 0; i < badgeLabelList.size(); ++i)
	{
		row9Layout->addWidget(badgeLabelList[i]);
		row9Layout->addWidget(badgeWidgets[i]);
	}

	row9Layout->addStretch();  // 添加伸缩项

	// 外部包裹的容器
	container = new QWidget(w1);
	container->setObjectName("TabContainer");
	container->setStyleSheet(QString("#TabContainer{background-color: %1;}")
		.arg(DesignSystem::instance()->currentTheme().tabContainerColor.name()));
	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			container->setStyleSheet(QString("#TabContainer{background-color: %1;}")
				.arg(DesignSystem::instance()->currentTheme().tabContainerColor.name()));
		});
	QHBoxLayout* cLayout = new QHBoxLayout(container);
	cLayout->setContentsMargins(20, 20, 20, 20);

	// 第一行布局
	row1Layout->addWidget(labelList[0]);
	row1Layout->addWidget(toggleBtn);
	row1Layout->addWidget(labelList[1]);
	row1Layout->addWidget(progress);
	row1Layout->addWidget(spinner);
	row1Layout->addStretch();  // 让它贴左边

	// 第二行布局
	row2Layout->addWidget(labelList[3]);
	row2Layout->addWidget(animNum);
	row2Layout->addWidget(labelList[2]);
	row2Layout->addWidget(slider);
	row2Layout->addStretch();

	// 第四行布局
	row4Layout->addWidget(taskBtn);
	row4Layout->addStretch();

	// 第五行布局
	row5Layout->addWidget(comboLabel1);
	row5Layout->addWidget(comboLabel2);
	row5Layout->addWidget(combo2);
	row5Layout->addStretch();

	// 第六行布局
	row6Layout->addStretch();

	// 第七行布局
	row7Layout->addWidget(cardLabel);
	row7Layout->addWidget(card);
	row7Layout->addStretch();

	QHBoxLayout* row8Layout = new QHBoxLayout();
	row8Layout->setSpacing(1);
	row8Layout->setContentsMargins(0, 0, 0, 0);
	// 创建卡片模型
	auto cardModel = QSharedPointer<DownloadCardModel>::create();
	cardModel->setTitle("示例视频标题");
	cardModel->setCoverUrl(QUrl("https://example.com/cover.jpg"));
	cardModel->setDuration("12:34");
	cardModel->setPublishTime(QDateTime::currentDateTime().addDays(-2));
	cardModel->setPublisher("视频发布者");
	cardModel->setVideoSize(1024 * 1024 * 150); // 150MB
	cardModel->setAudioSize(1024 * 1024 * 20);  // 20MB
	cardModel->setProgress(50);
	cardModel->setState(DownloadCardState::Downloading);

	// 创建卡片
	auto downloadCard = new DownloadCard(cardModel, this);

	// 连接信号
	connect(downloadCard, &DownloadCard::downloadClicked, this, [this, cardModel]() {
		// 开始下载逻辑
		// VideoDownloadRequest request = ...;
		// m_downloadManager->downloadVideo(request);
		});

	// 连接删除信号
	connect(downloadCard, &DownloadCard::deleteClicked, this, [this, downloadCard, row8Layout]() {
		// 从布局中移除并删除卡片
		row8Layout->removeWidget(downloadCard);
		downloadCard->deleteLater();
		});

	// 添加到布局中
	row8Layout->addWidget(downloadCard);

	// 添加到页面布局
	pageLay->addLayout(carouselLayout);
	pageLay->addLayout(row1Layout);
	pageLay->addLayout(row2Layout);
	pageLay->addWidget(skeletonDescBtn);
	pageLay->addLayout(row3Layout);
	pageLay->addLayout(row4Layout);
	pageLay->addLayout(row5Layout);
	pageLay->addLayout(row6Layout);
	pageLay->addLayout(row7Layout);
	pageLay->addLayout(row8Layout);
	pageLay->addLayout(row9Layout);
	pageLay->addWidget(container);
	pageLay->addStretch();
}

DownloadPage::~DownloadPage()
{
}

void DownloadPage::getVideoPlayUrl(DownloadTaskInfo& taskInfo)
{
	auto platformService = m_applicationController->getPlatformService();
	auto videoPlatfrom = platformService->getPlatform(taskInfo.request.platformId);
	taskInfo.request.videoPlayUrl = videoPlatfrom->getVideoPlayUrl(taskInfo.streamRequest);
}

void DownloadPage::getVideoCover(DownloadTaskInfo& taskInfo)
{
	auto platformService = m_applicationController->getPlatformService();
	auto videoPlatfrom = platformService->getPlatform(taskInfo.request.platformId);
	videoPlatfrom->getVideoCover(taskInfo);
}

void DownloadPage::createDownloadCards(const QList<VideoInfo>& videoInfoList)
{
	downloadReadyWidget->showLoading();

	// 创建任务列表
	auto sharedTaskList = QSharedPointer<QList<DownloadTaskInfo>>::create();
	sharedTaskList->reserve(videoInfoList.size());

	// 使用信号槽来在主线程中处理结果
	auto* watcher = new QFutureWatcher<DownloadTaskInfo>(this);

	connect(watcher, &QFutureWatcher<DownloadTaskInfo>::resultReadyAt, this,
		[this, sharedTaskList, watcher](int index) {
			// 这个槽会在主线程中被调用
			DownloadTaskInfo taskInfo = watcher->resultAt(index);
			sharedTaskList->append(taskInfo);
		});

	connect(watcher, &QFutureWatcher<DownloadTaskInfo>::finished, this,
		[this, watcher, sharedTaskList]() {
			downloadReadyWidget->addDownloadCards(std::move(*sharedTaskList));
			watcher->deleteLater();
		});

	// 使用 mapped 而不是 map，这样可以返回结果
	QFuture<DownloadTaskInfo> future = QtConcurrent::mapped(videoInfoList,
		[this](const VideoInfo& videoInfo) {
			// 在 worker 线程中处理
			DownloadTaskInfo taskInfo;
			taskInfo.taskId = taskInfo.request.generateTaskId();
			taskInfo.request.platformId = videoInfo.platformId;
			taskInfo.streamRequest.extraParams.insert(videoInfo.extraParams);
			taskInfo.videoInfo = videoInfo;

			// 网络请求，获取视频播放地址
			getVideoPlayUrl(taskInfo);
			getVideoCover(taskInfo);
			taskInfo.request.outputPath = "E:/CProject/" + videoInfo.title + ".mp4";

			return taskInfo;
		});

	watcher->setFuture(future);
}

void DownloadPage::resizeEvent(QResizeEvent* event)
{
	container->setFixedWidth(width() - 40);
	container->setMinimumHeight(static_cast<int>(width() * 9.0 / 16.0));
}

// 添加任务状态改变处理函数
void DownloadPage::onTaskStateChanged(const QString& taskId, ContainerState newState)
{
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
	// 更新任务信息中的文件路径
	DownloadTaskInfo taskInfo = downloadingWidget->getTaskInfo(taskId);
	if (!taskInfo.taskId.isEmpty()) {
		taskInfo.request.outputPath = filePath;
		taskInfo.status = Completed;
		// 转移到已下载容器
		onTaskStateChanged(taskId, ContainerState::Downloaded);
	}
}