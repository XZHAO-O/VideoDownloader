#include "DownloadPage.h"
#include <QVBoxLayout>
#include <QLabel>
#include "AntToggleButton.h"
#include "SlideStackedWidget.h"
#include "MaterialProgressBar.h"
#include <QList>
#include <QTimer>
#include <QScrollArea>
#include <QRegularExpression>
#include <QApplication>
#include "MaterialSpinner.h"
#include "AntRadioButton.h"
#include "AntSlider.h"
#include "NoDataWidget.h"
#include "AnimatedNumber.h"
#include "AntButton.h"
#include "NotificationManager.h"
#include "AntNumberInput.h"
#include "AntDoubleNumberInput.h"
#include "AntComboBox.h"
#include "TagWidget.h"
#include "CardWidget.h"
#include "QrCodeWidget.h"
#include "FlowLayout.h"
#include "DrawerWidget.h"
#include "BadgeWidget.h"
#include "AntChatListView.h"
#include "AntCellWidget.h"
#include "StyleSheet.h"
#include "PaginationWidget.h"
#include "AntTreeView.h"
#include "DownloadCard.h"
#include "DownloadManager.h"
#include "ApplicationController.h"

DownloadPage::DownloadPage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
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

	// 视图页
	initViewPage();

	// 流式布局
	QWidget* w3 = new QWidget(this);
	FlowLayout* flowLay = new FlowLayout(w3, 10, 6);
	QIcon svgIcon(":/Imgs/git.svg");
	for (int i = 0; i < 50; ++i)
	{
		QLabel* label = new QLabel(w3);
		label->setFixedSize(50, 50);
		label->setPixmap(svgIcon.pixmap(50, 50));
		flowLay->addWidget(label);
	}

	// 暂无数据
	NoDataWidget* noData = new NoDataWidget(this);

	downloadQueuePage = new DownloadQueuePage(m_downloadManager, this);  // 传递参数
	downloadingWidget = new DownloadingWidget(m_downloadManager, this);  // 传递参数
	downloadedWidget = new DownloadedWidget(m_downloadManager, this);    // 传递参数

	// 添加标签项
	tabWidget->addTab(downloadQueuePage, "待下载");
	tabWidget->addTab(downloadingWidget, "下载中");
	tabWidget->addTab(downloadedWidget, "已下载");
	tabWidget->addTab(scrollArea1, "常用控件");
	tabWidget->addTab(scrollArea2, "视图控件");
	tabWidget->addTab(w3, "流式布局");
	tabWidget->addTab(noData, "暂无数据");

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

	// 单选按钮
	AntRadioButton* radioBtn1 = new AntRadioButton(this);
	radioBtn1->setText("单选按钮1");
	AntRadioButton* radioBtn2 = new AntRadioButton(this);
	radioBtn2->setText("单选按钮2");

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
	QStringList topItems1 = { "水果", "蔬菜", "饮料", "汤", "零食", "烘焙", "速食" };
	AntComboBox* combo1 = new AntComboBox("请选择", topItems1, this);
	combo1->setFixedSize(185, 48);
	// 多层级下拉框
	QLabel* comboLabel2 = new QLabel("多层级下拉框", this);
	QStringList topItems2 = { "水果", "蔬菜", "饮料" };
	//QMap<QString, QStringList> subItemMap = {
	//	{ "水果", {"苹果", "香蕉", "西瓜"} },
	//	{ "蔬菜", {"白菜", "萝卜", "西红柿"} },
	//	{ "饮料", {"可乐", "雪碧", "果汁"} }
	//};
	AntComboBox* combo2 = new AntComboBox("请选择", topItems2, this);
	combo2->setFixedSize(185, 48);

	// 让下拉框遮罩跟随页面大小变化
	connect(this, &DownloadPage::resized, this, [=](int w, int h)
		{
			combo1->getMask()->resize(w, h);
			combo2->getMask()->resize(w, h);
		});

	// 下拉框子菜单定位
	connect(this, &DownloadPage::windowMoved, this, [combo1, combo2](QPoint globalPos)
		{
			auto movePopups = [globalPos, combo1, combo2](AntComboBox* combo)
				{
					for (PopupViewController* popup : combo->popupViewList())
					{
						// 单层级下拉框
						if (combo == combo1)
						{
							combo->popupViewList()[0]->follow(combo1);
						}
						// 多层下级拉框
						if (combo == combo2)
						{
							combo->popupViewList()[0]->follow(combo2);
							//combo2->popupViewList()[1]->follow(combo->popupViewList()[0], PopupViewController::TopRight);
						}
					}
				};
			movePopups(combo1);
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

	// 二维码
	QrCodeWidget* qrCode = new QrCodeWidget(this);
	qrCode->setMinimumSize(QSize(240, 240));
	qrCode->setData(QString("https://ant-design.antgroup.com/index-cn"));
	QLabel* qrCodeLabel = new QLabel("二维码", this);

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
	row2Layout->addWidget(radioBtn1);
	row2Layout->addWidget(radioBtn2);
	row2Layout->addWidget(labelList[2]);
	row2Layout->addWidget(slider);
	row2Layout->addStretch();

	// 第四行布局
	row4Layout->addWidget(taskBtn);
	row4Layout->addStretch();

	// 第五行布局
	row5Layout->addWidget(comboLabel1);
	row5Layout->addWidget(combo1);
	row5Layout->addWidget(comboLabel2);
	row5Layout->addWidget(combo2);
	row5Layout->addStretch();

	// 第六行布局
	row6Layout->addStretch();

	// 第七行布局
	row7Layout->addWidget(cardLabel);
	row7Layout->addWidget(card);
	row7Layout->addWidget(qrCodeLabel);
	row7Layout->addWidget(qrCode);
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
	cardModel->setState(DownloadCardState::Pending);

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
	auto platformService = m_downloadManager->getAppController()->getPlatformService();
	auto videoPlatfrom = platformService->getPlatform(taskInfo.request.platformId);
	taskInfo.request.videoPlayUrl = videoPlatfrom->getVideoPlayUrl(taskInfo.streamRequest);
}

void DownloadPage::createDownloadCards(QList<VideoInfo> videoInfoList)
{
	for (const auto& videoInfo : videoInfoList)
	{
		//创建任务信息
		DownloadTaskInfo taskInfo;
		taskInfo.taskId = taskInfo.request.generateTaskId();
		taskInfo.request.platformId = videoInfo.platformId;
		//存储请求参数到任务信息中
		taskInfo.streamRequest.extraParams.insert(videoInfo.extraParams);
		//taskInfo.streamRequest.extraParams["qn"] = "80";
		//根据请求参数获取视频地址
		getVideoPlayUrl(taskInfo);
		//taskInfo.videoId = videoInfo.videoId;

		// 创建卡片模型
		auto cardModel = QSharedPointer<DownloadCardModel>::create();
		cardModel->setTitle(videoInfo.title);
		cardModel->setCoverUrl(videoInfo.thumbnailUrl);
		cardModel->setDuration(videoInfo.duration);
		cardModel->setPublishTime(videoInfo.uploadDate);
		cardModel->setPublisher(videoInfo.author);
		cardModel->setVideoSize(0);
		cardModel->setAudioSize(0);
		cardModel->setState(DownloadCardState::Pending);

		// 创建卡片
		downloadQueuePage->addDownloadCard(taskInfo, new DownloadCard(cardModel, this));
	}
}

void DownloadPage::resizeEvent(QResizeEvent* event)
{
	container->setFixedWidth(width() - 40);
	container->setMinimumHeight(static_cast<int>(width() * 9.0 / 16.0));
}

void DownloadPage::initViewPage()
{
	scrollArea2 = new AntScrollArea(AntScrollArea::ScrollVertical, this);
	QWidget* w2 = new QWidget(this);
	scrollArea2->addWidget(w2);

	// 创建聊天项数据
	QVector<AntChatListView::ChatItem> chatItems = {
	{":/Imgs/bee.png", "张三", "你好，最近怎么样？", "10:30 AM", false},
	{":/Imgs/bee.png", "李四", "我很好，谢谢！你呢？", "10:31 AM", true},
	{":/Imgs/bee.png", "王五", "我们今天见面吗？", "10:32 AM", false},
	{":/Imgs/bee.png", "赵六", "今天晚上有空吗？", "10:33 AM", false},
	{":/Imgs/bee.png", "孙七", "今晚八点见！", "10:34 AM", true},
	{":/Imgs/bee.png", "周八", "好的，八点见！", "10:35 AM", false},
	{":/Imgs/bee.png", "吴九", "你最近在忙什么？", "10:36 AM", false},
	{":/Imgs/bee.png", "郑十", "最近工作挺忙的，快累死了", "10:37 AM", true},
	{":/Imgs/bee.png", "冯十一", "加油！工作顺利啊！", "10:38 AM", false},
	{":/Imgs/bee.png", "陈十二", "谢谢，努力！", "10:39 AM", true}
	};
	// 创建你的自定义列表视图
	AntChatListView* chatList = new AntChatListView(w2);
	// 用数据创建模型
	QStandardItemModel* listModel = chatList->createModel(chatItems);
	// 给视图设置模型
	chatList->setModel(listModel);
	chatList->setFixedHeight(600);
	// 视图页布局
	QVBoxLayout* w2Lay = new QVBoxLayout(w2);
	QLabel* listViewLab = new QLabel("列表视图", w2);
	listViewLab->setFixedHeight(20);
	w2Lay->setContentsMargins(10, 0, 10, 0);
	w2Lay->setSpacing(10);

	w2Lay->addWidget(listViewLab);
	w2Lay->addWidget(chatList);
}