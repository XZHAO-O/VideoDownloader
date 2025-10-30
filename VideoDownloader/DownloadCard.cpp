#include "DownloadCard.h"

#include <QLabel>
#include <QNetworkReply>

#include "AntButton.h"
#include "AntCellWidget.h"
#include "DesignSystem.h"
#include "AntTooltipManager.h"
#include "MaterialProgressBar.h"
#include "SingleLevelComboBox.h"
#include "DownloadTaskInfo.h"
#include "Instrumentor.h"

DownloadCard::DownloadCard(QSharedPointer<DownloadCardModel> model, QWidget* parent)
	: QWidget(parent)
	, m_model(model)
	, m_currentState(model->state())
{
	BENCHMARKING_FUNCTION();

	setObjectName("DownloadCard");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumHeight(160);
	setAttribute(Qt::WA_Hover, true);

	initUI();
	updateTextColors();
	initConnections();
	initModelConnections();

	onModelChanged();

	// 添加主题变化监听，使用与AntButton相同的模式
	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
		updateTextColors();
		update();
		});
}

DownloadCard::~DownloadCard()
{
	qDebug() << "=== DownloadCard Destroying - Third Party Components ===";
}

void DownloadCard::setModel(QSharedPointer<DownloadCardModel> model)
{
	BENCHMARKING_FUNCTION();

	disconnect(m_model.get(), nullptr, this, nullptr);

	m_model = model;

	//更新资源
	m_isCoverLoaded = false;

	onModelChanged();
}

QSize DownloadCard::sizeHint() const
{
	return QSize(680, 160);
}

QSize DownloadCard::minimumSizeHint() const
{
	return QSize(400, 160);
}

void DownloadCard::mousePressEvent(QMouseEvent* event)
{
	QWidget::mousePressEvent(event);
}

void DownloadCard::enterEvent(QEnterEvent* event)
{
	m_hovered = true;
	update();
	QWidget::enterEvent(event);
}

void DownloadCard::leaveEvent(QEvent* event)
{
	m_hovered = false;
	update();
	QWidget::leaveEvent(event);
}

void DownloadCard::paintEvent(QPaintEvent* event)
{
	BENCHMARKING_FUNCTION();
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	QRect bgRect = rect();
	QColor bgColor = DesignSystem::instance()->currentTheme().cardBackgroundColor;

	if (m_hovered) {
		bgColor = bgColor.lighter(105);
	}

	painter.setBrush(bgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(bgRect, 8, 8);

	// 绘制边框
	if (m_hovered) {
		QPen borderPen(DesignSystem::instance()->primaryColor());
		borderPen.setWidth(1);
		painter.setPen(borderPen);
		painter.setBrush(Qt::NoBrush);
		painter.drawRoundedRect(bgRect.adjusted(1, 1, -1, -1), 8, 8);
	}

	QWidget::paintEvent(event);
}

void DownloadCard::onModelChanged()
{
	BENCHMARKING_FUNCTION();

	updateUI();
}

void DownloadCard::onCoverClicked()
{
	if (m_model->state() == DownloadCardState::Downloaded) {
		emit previewClicked();

		// 如果预览窗口已存在，先关闭它
		if (m_previewWindow && !m_previewWindow.isNull()) {
			m_previewWindow->close();
			m_previewWindow.clear();
		}

		// 创建新的预览窗口
		m_previewWindow = QSharedPointer<VideoPreviewWindow>::create();
		m_previewWindow->setVideoFile(m_model->filePath());
		m_previewWindow->show();
	}
}

void DownloadCard::onTitleClicked()
{
	onCoverClicked();
}

void DownloadCard::onVideoQualityChanged(const QString& quality)
{
	// 转换质量等级
	VideoQualityLevel level = VideoQualityLevel::High;
	if (quality == "480p") level = VideoQualityLevel::Low;
	else if (quality == "720p") level = VideoQualityLevel::Medium;
	else if (quality == "1080p") level = VideoQualityLevel::High;
	else if (quality == "4K") level = VideoQualityLevel::Ultra;
	else if (quality == "原画") level = VideoQualityLevel::Original;

	m_model->setVideoQuality(level);
}

void DownloadCard::onAudioQualityChanged(const QString& quality)
{
	// 转换质量等级
	AudioQualityLevel level = AudioQualityLevel::High;
	if (quality == "低音质") level = AudioQualityLevel::Low;
	else if (quality == "中音质") level = AudioQualityLevel::Medium;
	else if (quality == "高音质") level = AudioQualityLevel::High;
	else if (quality == "无损") level = AudioQualityLevel::Ultra;

	m_model->setAudioQuality(level);
}

void DownloadCard::initUI()
{
	BENCHMARKING_FUNCTION();

	// 根据状态初始化不同的UI
	switch (m_model->state())
	{
	case DownloadCardState::Pending:
		initPendingUI();
		break;
	case DownloadCardState::Downloading:
		initDownloadingUI();
		break;
	case DownloadCardState::Downloaded:
		initDownloadedUI();
		break;
	}
}

void DownloadCard::initPendingUI()
{
	BENCHMARKING_FUNCTION();
	// 主布局
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setSpacing(12);
	m_mainLayout->setContentsMargins(16, 8, 16, 8);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);

	QVBoxLayout* coverLayout = new QVBoxLayout(m_coverContainer);
	coverLayout->setContentsMargins(0, 0, 0, 0);
	coverLayout->setSpacing(0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(DesignSystem::instance()->borderColor().name()));
	m_coverLabel->setScaledContents(true);
	m_coverLabel->setText(QString("<a href='preview' style='text-decoration:none; color:transparent;'> </a>"));

	m_playIcon = new QLabel(m_coverContainer);
	m_playIcon->setFixedSize(40, 40);
	m_playIcon->setStyleSheet("QLabel{"
		"background-color: rgba(0, 0, 0, 0.6);"
		"border-radius: 20px;"
		"}");
	m_playIcon->setAlignment(Qt::AlignCenter);
	m_playIcon->setPixmap(QPixmap(":/Imgs/play.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_playIcon->setVisible(false);

	coverLayout->addWidget(m_coverLabel);
	coverLayout->addWidget(m_playIcon, 0, Qt::AlignCenter);

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局（标题 + 操作按钮）
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	// 使用 AntCellWidget 替换原来的 QLabel
	m_titleCell = new AntCellWidget("视频标题", this);
	auto theme = DesignSystem::instance()->currentTheme();
	m_titleCell->getBtn()->setStyleSheet(
		QString("QPushButton {"
			"    background-color: transparent;"
			"    border: none;"
			"    color: %1;"
			"    font-size: 14px;"
			"    text-align: left;"
			"    padding: 0px;"
			"    margin: 0px;"
			"}"
			"QPushButton:hover {"
			"    color: %2;"
			"    background-color: transparent;"
			"}")
		.arg(theme.primaryTextColor.name())
		.arg(theme.primaryColor.name())
	);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	// 创建待下载状态按钮
	m_downloadBtn = new AntButton("下载", 10, this);
	m_downloadBtn->setFixedSize(70, 32);

	m_videoDownloadBtn = new AntButton("视频", 10, this);
	m_videoDownloadBtn->setFixedSize(70, 32);

	m_audioDownloadBtn = new AntButton("音频", 10, this);
	m_audioDownloadBtn->setFixedSize(70, 32);

	m_closeBtn = new AntButton("×", 10, this);
	m_closeBtn->setFixedSize(32, 32);

	// 添加到操作布局
	m_actionLayout->addWidget(m_downloadBtn);
	m_actionLayout->addWidget(m_videoDownloadBtn);
	m_actionLayout->addWidget(m_audioDownloadBtn);
	m_actionLayout->addWidget(m_closeBtn);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局（大小信息 + 质量选择）
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("视频: 0 MB  音频: 0 MB", this);

	// 质量选择
	QWidget* qualityWidget = new QWidget(this);
	QHBoxLayout* qualityLayout = new QHBoxLayout(qualityWidget);
	qualityLayout->setSpacing(8);
	qualityLayout->setContentsMargins(0, 0, 0, 0);

	QStringList qualityList = { "480p", "720p", "1080p", "4K", "原画", "8K" };
	m_videoQualityCombo = new SingleLevelComboBox("画质", qualityList, this);
	m_videoQualityCombo->setFixedSize(90, 28);

	QStringList audioQualityList = { "低音质", "中音质", "高音质", "无损" };
	m_audioQualityCombo = new SingleLevelComboBox("音质", audioQualityList, this);
	m_audioQualityCombo->setFixedSize(90, 28);

	qualityLayout->addWidget(m_videoQualityCombo);
	qualityLayout->addWidget(m_audioQualityCombo);
	qualityLayout->addStretch();

	m_middleLayout->addWidget(m_sizeLabel);
	m_middleLayout->addWidget(qualityWidget);
	m_middleLayout->addStretch();

	// 底部布局
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	// 第一行：时间信息
	QHBoxLayout* timeLayout = new QHBoxLayout();
	timeLayout->setSpacing(12);
	timeLayout->setContentsMargins(0, 0, 0, 0);

	m_timeLabel = new QLabel("发布时间 · 时长", this);

	timeLayout->addWidget(m_timeLabel);
	timeLayout->addStretch();

	// 第二行：发布者信息
	QHBoxLayout* publisherLayout = new QHBoxLayout();
	publisherLayout->setSpacing(12);
	publisherLayout->setContentsMargins(0, 0, 0, 0);

	m_publisherLabel = new QLabel("发布者", this);

	publisherLayout->addWidget(m_publisherLabel);
	publisherLayout->addStretch();

	// 组装底部布局
	m_bottomLayout->addLayout(timeLayout);
	m_bottomLayout->addLayout(publisherLayout);

	// 组装内容布局
	m_contentLayout->addLayout(m_headerLayout);
	m_contentLayout->addLayout(m_middleLayout);
	m_contentLayout->addLayout(m_bottomLayout);

	// 组装主布局
	m_mainLayout->addWidget(m_coverContainer);
	m_mainLayout->addWidget(contentWidget, 1);
}

void DownloadCard::initDownloadingUI()
{
	BENCHMARKING_FUNCTION();
	// 主布局
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setSpacing(12);
	m_mainLayout->setContentsMargins(16, 8, 16, 8);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);

	QVBoxLayout* coverLayout = new QVBoxLayout(m_coverContainer);
	coverLayout->setContentsMargins(0, 0, 0, 0);
	coverLayout->setSpacing(0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(DesignSystem::instance()->borderColor().name()));
	m_coverLabel->setScaledContents(true);

	coverLayout->addWidget(m_coverLabel);

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局（标题 + 操作按钮）
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	// 使用 AntCellWidget
	m_titleCell = new AntCellWidget("视频标题", this);
	auto theme = DesignSystem::instance()->currentTheme();
	m_titleCell->getBtn()->setStyleSheet(
		QString("QPushButton {"
			"    background-color: transparent;"
			"    border: none;"
			"    color: %1;"
			"    font-size: 14px;"
			"    text-align: left;"
			"    padding: 0px;"
			"    margin: 0px;"
			"}"
			"QPushButton:hover {"
			"    color: %2;"
			"    background-color: transparent;"
			"}")
		.arg(theme.primaryTextColor.name())
		.arg(theme.primaryColor.name())
	);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	// 创建下载中状态按钮
	m_pauseBtn_downloading = new AntButton("暂停", 10, this);
	m_pauseBtn_downloading->setFixedSize(70, 32);

	m_openFolderBtn_downloading = new AntButton("文件夹", 10, this);
	m_openFolderBtn_downloading->setFixedSize(70, 32);

	m_deleteBtn_downloading = new AntButton("删除", 10, this);
	m_deleteBtn_downloading->setFixedSize(70, 32);

	// 添加到操作布局
	m_actionLayout->addWidget(m_pauseBtn_downloading);
	m_actionLayout->addWidget(m_openFolderBtn_downloading);
	m_actionLayout->addWidget(m_deleteBtn_downloading);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局（大小信息）
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("视频: 0 MB  音频: 0 MB", this);

	m_middleLayout->addWidget(m_sizeLabel);
	m_middleLayout->addStretch();

	// 底部布局 - 进度信息
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	// 进度信息区域
	QWidget* progressWidget = new QWidget(this);
	QVBoxLayout* progressLayout = new QVBoxLayout(progressWidget);
	progressLayout->setSpacing(4);
	progressLayout->setContentsMargins(0, 0, 0, 16);

	QHBoxLayout* progressInfoLayout = new QHBoxLayout();
	progressInfoLayout->setSpacing(8);
	progressInfoLayout->setContentsMargins(0, 0, 0, 2);

	// 进度信息标签（已下载/总共）
	m_progressInfoLabel = new QLabel("0MB/0MB", this);
	m_speedLabel = new QLabel("0 B/s", this);

	progressInfoLayout->addWidget(m_progressInfoLabel);
	progressInfoLayout->addStretch();
	progressInfoLayout->addWidget(m_speedLabel);

	m_progressBar = new MaterialProgressBar(this);
	m_progressBar->setFixedHeight(12);

	progressLayout->addLayout(progressInfoLayout);
	progressLayout->addWidget(m_progressBar);

	// 组装底部布局
	m_bottomLayout->addWidget(progressWidget);

	// 组装内容布局
	m_contentLayout->addLayout(m_headerLayout);
	m_contentLayout->addLayout(m_middleLayout);
	m_contentLayout->addLayout(m_bottomLayout);

	// 组装主布局
	m_mainLayout->addWidget(m_coverContainer);
	m_mainLayout->addWidget(contentWidget, 1);
}

void DownloadCard::initDownloadedUI()
{
	BENCHMARKING_FUNCTION();
	// 主布局
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setSpacing(12);
	m_mainLayout->setContentsMargins(16, 8, 16, 8);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);

	QVBoxLayout* coverLayout = new QVBoxLayout(m_coverContainer);
	coverLayout->setContentsMargins(0, 0, 0, 0);
	coverLayout->setSpacing(0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(DesignSystem::instance()->borderColor().name()));
	m_coverLabel->setScaledContents(true);
	m_coverLabel->setText(QString("<a href='preview' style='text-decoration:none; color:transparent;'> </a>"));

	m_playIcon = new QLabel(m_coverContainer);
	m_playIcon->setFixedSize(40, 40);
	m_playIcon->setStyleSheet("QLabel{"
		"background-color: rgba(0, 0, 0, 0.6);"
		"border-radius: 20px;"
		"}");
	m_playIcon->setAlignment(Qt::AlignCenter);
	m_playIcon->setPixmap(QPixmap(":/Imgs/play.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_playIcon->setVisible(true);

	coverLayout->addWidget(m_coverLabel);
	coverLayout->addWidget(m_playIcon, 0, Qt::AlignCenter);

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局（标题 + 操作按钮）
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	// 使用 AntCellWidget
	m_titleCell = new AntCellWidget("视频标题", this);
	auto theme = DesignSystem::instance()->currentTheme();
	m_titleCell->getBtn()->setStyleSheet(
		QString("QPushButton {"
			"    background-color: transparent;"
			"    border: none;"
			"    color: %1;"
			"    font-size: 14px;"
			"    text-align: left;"
			"    padding: 0px;"
			"    margin: 0px;"
			"}"
			"QPushButton:hover {"
			"    color: %2;"
			"    background-color: transparent;"
			"}")
		.arg(theme.primaryTextColor.name())
		.arg(theme.primaryColor.name())
	);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	// 创建已下载状态按钮
	m_openUrlBtn_downloaded = new AntButton("打开链接", 10, this);
	m_openUrlBtn_downloaded->setFixedSize(70, 32);

	m_openFolderBtn_downloaded = new AntButton("文件夹", 10, this);
	m_openFolderBtn_downloaded->setFixedSize(70, 32);

	m_deleteBtn_downloaded = new AntButton("删除", 10, this);
	m_deleteBtn_downloaded->setFixedSize(70, 32);

	// 添加到操作布局
	m_actionLayout->addWidget(m_openUrlBtn_downloaded);
	m_actionLayout->addWidget(m_openFolderBtn_downloaded);
	m_actionLayout->addWidget(m_deleteBtn_downloaded);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局（大小信息）
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("视频: 0 MB  音频: 0 MB", this);

	m_middleLayout->addWidget(m_sizeLabel);
	m_middleLayout->addStretch();

	// 底部布局
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	// 时间信息
	QHBoxLayout* timeLayout = new QHBoxLayout();
	timeLayout->setSpacing(12);
	timeLayout->setContentsMargins(0, 0, 0, 0);

	m_timeLabel = new QLabel("下载完成时间", this);

	timeLayout->addWidget(m_timeLabel);
	timeLayout->addStretch();

	// 组装底部布局
	m_bottomLayout->addLayout(timeLayout);

	// 组装内容布局
	m_contentLayout->addLayout(m_headerLayout);
	m_contentLayout->addLayout(m_middleLayout);
	m_contentLayout->addLayout(m_bottomLayout);

	// 组装主布局
	m_mainLayout->addWidget(m_coverContainer);
	m_mainLayout->addWidget(contentWidget, 1);
}

void DownloadCard::initConnections()
{
	BENCHMARKING_FUNCTION();
	// 公共连接
	connect(m_coverLabel, &QLabel::linkActivated, this, &DownloadCard::onCoverClicked);

	connect(m_titleCell->getBtn(), &QPushButton::clicked, this, &DownloadCard::onTitleClicked);

	// 根据状态初始化特定连接
	switch (m_currentState)
	{
	case DownloadCardState::Pending:

		connect(m_videoQualityCombo, SIGNAL(currentTextChanged(QString)),
			this, SLOT(onVideoQualityChanged(QString)));
		connect(m_audioQualityCombo, SIGNAL(currentTextChanged(QString)),
			this, SLOT(onAudioQualityChanged(QString)));

		connect(m_downloadBtn, &AntButton::clicked, this, &DownloadCard::downloadClicked);

		connect(m_videoDownloadBtn, &AntButton::clicked, this, &DownloadCard::videoDownloadClicked);

		connect(m_audioDownloadBtn, &AntButton::clicked, this, &DownloadCard::audioDownloadClicked);

		connect(m_closeBtn, &AntButton::clicked, this, &DownloadCard::deleteClicked);
		break;

	case DownloadCardState::Downloading:

		connect(m_pauseBtn_downloading, &AntButton::clicked, this, [this]() {
			emit pauseClicked();
			});

		connect(m_openFolderBtn_downloading, &AntButton::clicked, this, &DownloadCard::openFolderClicked);

		connect(m_deleteBtn_downloading, &AntButton::clicked, this, &DownloadCard::deleteClicked);
		break;

	case DownloadCardState::Downloaded:

		connect(m_openUrlBtn_downloaded, &AntButton::clicked, this, &DownloadCard::openUrlClicked);

		connect(m_openFolderBtn_downloaded, &AntButton::clicked, this, &DownloadCard::openFolderClicked);

		connect(m_deleteBtn_downloaded, &AntButton::clicked, this, &DownloadCard::deleteClicked);
		break;
	}
}

void DownloadCard::initModelConnections()
{
	BENCHMARKING_FUNCTION();

	connect(m_model.get(), &DownloadCardModel::titleChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::coverUrlChanged, this, &DownloadCard::onModelChanged);

	// 根据状态连接特定信号
	switch (m_currentState)
	{
	case DownloadCardState::Pending:

		connect(m_model.get(), &DownloadCardModel::videoQualityChanged, this, &DownloadCard::onModelChanged);

		connect(m_model.get(), &DownloadCardModel::audioQualityChanged, this, &DownloadCard::onModelChanged);
		break;

	case DownloadCardState::Downloading:

		connect(m_model.get(), &DownloadCardModel::progressChanged, this, &DownloadCard::onModelChanged);

		connect(m_model.get(), &DownloadCardModel::downloadSpeedChanged, this, &DownloadCard::onModelChanged);
		break;

	case DownloadCardState::Downloaded:
		// 已下载状态特有的连接
		break;
	}
}

void DownloadCard::onDownloadProgress(const QString& progressInfo, int progress)
{
	model()->setProgressInfo(progressInfo);
	model()->setProgress(progress);
}

void DownloadCard::updateUI()
{
	BENCHMARKING_FUNCTION();

	// 更新基本信息
	m_titleCell->getBtn()->setText(m_model->title());
	m_sizeLabel->setText(QString("视频: %1  音频: %2")
		.arg(m_model->formattedVideoSize())
		.arg(m_model->formattedAudioSize()));

	// 根据状态更新UI
	switch (m_model->state())
	{
	case DownloadCardState::Pending:
		updatePendingUI();
		break;
	case DownloadCardState::Downloading:
		updateDownloadingUI();
		break;
	case DownloadCardState::Downloaded:
		updateDownloadedUI();
		break;
	}

	// 加载封面图片
	if (m_isCoverLoaded)
		return;
	const QByteArray& data = m_model->cover();
	if (!data.isEmpty())
	{
		QPixmap pixmap;
		pixmap.loadFromData(data);
		m_coverLabel->setPixmap(pixmap.scaled(140, 105, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
		m_isCoverLoaded = true;
	}
}

void DownloadCard::updatePendingUI()
{
	BENCHMARKING_FUNCTION();
	m_timeLabel->setText(QString("%1 · %2")
		.arg(m_model->formattedPublishTime())
		.arg(m_model->formattedDuration()));
	m_publisherLabel->setText(m_model->publisher());

	// 更新质量选择
	if (m_videoQualityCombo) {
		// 根据当前视频质量设置组合框
		// 这里需要根据m_model->videoQuality()来设置当前选项
	}
	if (m_audioQualityCombo) {
		// 根据当前音频质量设置组合框
	}
}

void DownloadCard::updateDownloadingUI()
{
	BENCHMARKING_FUNCTION();

	m_progressBar->setValue(m_model->progress());

	m_speedLabel->setText(m_model->formattedDownloadSpeed());

	m_progressInfoLabel->setText(m_model->progressInfo());
}

void DownloadCard::updateDownloadedUI()
{
	BENCHMARKING_FUNCTION();

	m_timeLabel->setText(QString("下载完成: %1").arg(m_model->formattedPublishTime()));

	m_playIcon->setVisible(true);
}

void DownloadCard::updateTextColors()
{
	BENCHMARKING_FUNCTION();
	auto theme = DesignSystem::instance()->currentTheme();

	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(theme.borderColor.name()));

	m_titleCell->getBtn()->setStyleSheet(
		QString("QPushButton {"
			"    background-color: transparent;"
			"    border: none;"
			"    color: %1;"
			"    font-size: 14px;"
			"    text-align: left;"
			"    padding: 0px;"
			"    margin: 0px;"
			"}"
			"QPushButton:hover {"
			"    color: %2;"
			"    background-color: transparent;"
			"}")
		.arg(theme.primaryTextColor.name())
		.arg(theme.primaryColor.name())
	);

	m_sizeLabel->setStyleSheet(QString("QLabel{"
		"font-size: 12px;"
		"color: %1;"
		"}").arg(theme.secondaryTextColor.name()));

	switch (m_currentState)
	{
	case DownloadCardState::Pending:

		m_timeLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.tertiaryTextColor.name()));

		m_publisherLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.tertiaryTextColor.name()));
		break;
	case DownloadCardState::Downloading:

		m_progressInfoLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.secondaryTextColor.name()));

		m_speedLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.secondaryTextColor.name()));
		break;
	case DownloadCardState::Downloaded:
		break;
	}
}