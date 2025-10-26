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

DownloadCard::DownloadCard(QSharedPointer<DownloadCardModel> model, QWidget* parent)
	: QWidget(parent)
	, m_model(model)
{
	setObjectName("DownloadCard");
	// 移除固定大小，使用尺寸策略
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumHeight(160); // 恢复适当高度
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
	if (m_model == model || !model) return;

	if (m_model)
	{
		disconnect(m_model.get(), nullptr, this, nullptr);
	}

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
	if (!m_model) return;

	updateUI();
	updateButtonStates();
}

void DownloadCard::onCoverClicked()
{
	if (m_model && m_model->state() == DownloadCardState::Downloaded) {
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
	if (!m_model) return;

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
	if (!m_model) return;

	// 转换质量等级
	AudioQualityLevel level = AudioQualityLevel::High;
	if (quality == "低音质") level = AudioQualityLevel::Low;
	else if (quality == "中音质") level = AudioQualityLevel::Medium;
	else if (quality == "高音质") level = AudioQualityLevel::High;
	else if (quality == "无损") level = AudioQualityLevel::Ultra;

	m_model->setAudioQuality(level);
}

void DownloadCard::updateButtonStates()
{
	if (!m_model) return;

	// 根据状态更新按钮状态
	DownloadCardState state = m_model->state();

	// 待下载状态
	bool isPending = (state == DownloadCardState::Pending);
	m_videoQualityCombo->setVisible(isPending);
	m_audioQualityCombo->setVisible(isPending);
	m_downloadBtn->setVisible(isPending);
	m_videoDownloadBtn->setVisible(isPending);
	m_audioDownloadBtn->setVisible(isPending);
	m_closeBtn->setVisible(isPending);

	// 下载中状态
	bool isDownloading = (state == DownloadCardState::Downloading);
	m_progressBar->setVisible(isDownloading);
	m_progressInfoLabel->setVisible(isDownloading); // 使用新的进度信息标签
	m_speedLabel->setVisible(isDownloading);
	m_pauseBtn_downloading->setVisible(isDownloading);
	m_openFolderBtn_downloading->setVisible(isDownloading);
	m_deleteBtn_downloading->setVisible(isDownloading);

	// 已下载状态
	bool isDownloaded = (state == DownloadCardState::Downloaded);
	m_openUrlBtn_downloaded->setVisible(isDownloaded);
	m_openFolderBtn_downloaded->setVisible(isDownloaded);
	m_deleteBtn_downloaded->setVisible(isDownloaded);

	// 更新暂停/继续按钮文本
	if (isDownloading) {
		m_pauseBtn_downloading->setText("暂停");
	}
}

// 修改initUI函数，移除所有硬编码的颜色，使用DesignSystem动态获取
void DownloadCard::initUI()
{
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
	// 使用DesignSystem获取边框颜色
	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(DesignSystem::instance()->borderColor().name()));
	m_coverLabel->setScaledContents(true);
	// 让封面可点击
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
	// 设置标题样式 - 只让文字变色，不要背景
	auto theme = DesignSystem::instance()->currentTheme();
	m_titleCell->getBtn()->setStyleSheet(
		QString("QPushButton {"
			"    background-color: transparent;"
			"    border: none;"
			"    color: %1;"
			"    font-size: 14px;"
			"    font-weight: bold;"
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

	// 创建下载中状态按钮（独立实例）
	m_pauseBtn_downloading = new AntButton("暂停", 10, this);
	m_pauseBtn_downloading->setFixedSize(70, 32);

	m_openFolderBtn_downloading = new AntButton("文件夹", 10, this);
	m_openFolderBtn_downloading->setFixedSize(70, 32);

	m_deleteBtn_downloading = new AntButton("删除", 10, this);
	m_deleteBtn_downloading->setFixedSize(70, 32);

	// 创建已下载状态按钮（独立实例）
	m_openUrlBtn_downloaded = new AntButton("打开链接", 10, this);
	m_openUrlBtn_downloaded->setFixedSize(70, 32);

	m_openFolderBtn_downloaded = new AntButton("文件夹", 10, this);
	m_openFolderBtn_downloaded->setFixedSize(70, 32);

	m_deleteBtn_downloaded = new AntButton("删除", 10, this);
	m_deleteBtn_downloaded->setFixedSize(70, 32);

	// 添加到操作布局
	m_actionLayout->addWidget(m_downloadBtn);
	m_actionLayout->addWidget(m_videoDownloadBtn);
	m_actionLayout->addWidget(m_audioDownloadBtn);
	m_actionLayout->addWidget(m_closeBtn);
	m_actionLayout->addWidget(m_pauseBtn_downloading);
	m_actionLayout->addWidget(m_openFolderBtn_downloading);
	m_actionLayout->addWidget(m_deleteBtn_downloading);
	m_actionLayout->addWidget(m_openUrlBtn_downloaded);
	m_actionLayout->addWidget(m_openFolderBtn_downloaded);
	m_actionLayout->addWidget(m_deleteBtn_downloaded);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局（大小信息 + 质量选择）
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("视频: 0 MB  音频: 0 MB", this);
	// 大小信息样式将在updateUI中动态设置

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
	// 时间信息样式将在updateUI中动态设置

	timeLayout->addWidget(m_timeLabel);
	timeLayout->addStretch();

	// 第二行：发布者信息
	QHBoxLayout* publisherLayout = new QHBoxLayout();
	publisherLayout->setSpacing(12);
	publisherLayout->setContentsMargins(0, 0, 0, 0);

	m_publisherLabel = new QLabel("发布者", this);
	// 发布者信息样式将在updateUI中动态设置

	publisherLayout->addWidget(m_publisherLabel);
	publisherLayout->addStretch();

	// 进度信息区域（在下载中状态时显示在发布者位置）
	QWidget* progressWidget = new QWidget(this);
	QVBoxLayout* progressLayout = new QVBoxLayout(progressWidget);
	progressLayout->setSpacing(4);
	progressLayout->setContentsMargins(0, 0, 0, 16);

	QHBoxLayout* progressInfoLayout = new QHBoxLayout();
	progressInfoLayout->setSpacing(8);
	progressInfoLayout->setContentsMargins(0, 0, 0, 2);

	// 新的进度信息标签（已下载/总共）
	m_progressInfoLabel = new QLabel("0MB/0MB", this);
	// 进度信息样式将在updateUI中动态设置

	m_speedLabel = new QLabel("0 B/s", this);
	// 速度信息样式将在updateUI中动态设置

	progressInfoLayout->addWidget(m_progressInfoLabel);
	progressInfoLayout->addStretch();
	progressInfoLayout->addWidget(m_speedLabel);

	m_progressBar = new MaterialProgressBar(this);
	m_progressBar->setFixedHeight(12);

	progressLayout->addLayout(progressInfoLayout);
	progressLayout->addWidget(m_progressBar);

	// 组装底部布局
	m_bottomLayout->addLayout(timeLayout);
	m_bottomLayout->addLayout(publisherLayout);
	m_bottomLayout->addWidget(progressWidget); // 进度条区域

	// 组装内容布局
	m_contentLayout->addLayout(m_headerLayout);
	m_contentLayout->addLayout(m_middleLayout);
	m_contentLayout->addLayout(m_bottomLayout);

	// 组装主布局
	m_mainLayout->addWidget(m_coverContainer);
	m_mainLayout->addWidget(contentWidget, 1);

	// 初始隐藏不需要的组件
	updateButtonStates();
}

void DownloadCard::initConnections()
{
	// 封面点击
	connect(m_coverLabel, &QLabel::linkActivated, this, &DownloadCard::onCoverClicked);

	// 标题点击 - 使用 AntCellWidget 的按钮点击信号
	connect(m_titleCell->getBtn(), &QPushButton::clicked, this, &DownloadCard::onTitleClicked);

	// 质量选择 - 使用旧的连接语法避免信号问题
	connect(m_videoQualityCombo, SIGNAL(currentTextChanged(QString)),
		this, SLOT(onVideoQualityChanged(QString)));
	connect(m_audioQualityCombo, SIGNAL(currentTextChanged(QString)),
		this, SLOT(onAudioQualityChanged(QString)));

	// 待下载状态按钮连接
	connect(m_downloadBtn, &AntButton::clicked, this, &DownloadCard::downloadClicked);
	connect(m_videoDownloadBtn, &AntButton::clicked, this, &DownloadCard::videoDownloadClicked);
	connect(m_audioDownloadBtn, &AntButton::clicked, this, &DownloadCard::audioDownloadClicked);
	connect(m_closeBtn, &AntButton::clicked, this, &DownloadCard::deleteClicked); // 关闭按钮触发删除

	// 下载中状态按钮连接
	connect(m_pauseBtn_downloading, &AntButton::clicked, this, [this]() {
		if (m_model && m_model->state() == DownloadCardState::Downloading) {
			emit pauseClicked();
		}
		else {
			emit resumeClicked();
		}
		});
	connect(m_openFolderBtn_downloading, &AntButton::clicked, this, &DownloadCard::openFolderClicked);
	connect(m_deleteBtn_downloading, &AntButton::clicked, this, &DownloadCard::deleteClicked);

	// 已下载状态按钮连接
	connect(m_openUrlBtn_downloaded, &AntButton::clicked, this, &DownloadCard::openUrlClicked);
	connect(m_openFolderBtn_downloaded, &AntButton::clicked, this, &DownloadCard::openFolderClicked);
	connect(m_deleteBtn_downloaded, &AntButton::clicked, this, &DownloadCard::deleteClicked);
}

void DownloadCard::initModelConnections()
{
	connect(m_model.get(), &DownloadCardModel::stateChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::progressChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::downloadSpeedChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::titleChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::coverUrlChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::videoQualityChanged, this, &DownloadCard::onModelChanged);
	connect(m_model.get(), &DownloadCardModel::audioQualityChanged, this, &DownloadCard::onModelChanged);
}

// 修改updateUI函数，在每次更新时动态设置颜色
void DownloadCard::updateUI()
{
	if (!m_model) return;

	// 更新基本信息 - 直接设置 AntCellWidget 的按钮文本
	m_titleCell->getBtn()->setText(m_model->title());
	m_sizeLabel->setText(QString("视频: %1  音频: %2")
		.arg(m_model->formattedVideoSize())
		.arg(m_model->formattedAudioSize()));
	m_timeLabel->setText(QString("%1 · %2")
		.arg(m_model->formattedPublishTime())
		.arg(m_model->formattedDuration()));
	m_publisherLabel->setText(m_model->publisher());

	// 根据状态更新UI
	switch (m_model->state()) {
	case DownloadCardState::Pending:
		updatePendingUI();
		break;
	case DownloadCardState::Downloading:
		updateDownloadingUI();
		break;
	case DownloadCardState::Downloaded:
		updateDownloadedUI();
		break;
	case DownloadCardState::Error:
		// 错误状态处理
		break;
	}

	// 加载封面图片
	if (m_isCoverLoaded)
		return;
	const QByteArray& data = m_model->cover();
	if (!data.isEmpty()) {
		QPixmap pixmap;
		pixmap.loadFromData(data);
		m_coverLabel->setPixmap(pixmap.scaled(140, 105, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
		m_isCoverLoaded = true;
	}

}

// 添加updateTextColors函数
void DownloadCard::updateTextColors()
{
	auto theme = DesignSystem::instance()->currentTheme();

	// 更新标题颜色 - 只让文字变色，不要背景
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

	// 更新大小信息颜色
	m_sizeLabel->setStyleSheet(QString("QLabel{"
		"font-size: 12px;"
		"color: %1;"
		"}").arg(theme.secondaryTextColor.name()));

	// 更新时间信息颜色
	m_timeLabel->setStyleSheet(QString("QLabel{"
		"font-size: 11px;"
		"color: %1;"
		"}").arg(theme.tertiaryTextColor.name()));

	// 更新发布者信息颜色
	m_publisherLabel->setStyleSheet(QString("QLabel{"
		"font-size: 11px;"
		"color: %1;"
		"}").arg(theme.tertiaryTextColor.name()));

	// 更新进度信息颜色
	m_progressInfoLabel->setStyleSheet(QString("QLabel{"
		"font-size: 11px;"
		"color: %1;"
		"}").arg(theme.secondaryTextColor.name()));

	// 更新速度信息颜色
	m_speedLabel->setStyleSheet(QString("QLabel{"
		"font-size: 11px;"
		"color: %1;"
		"}").arg(theme.secondaryTextColor.name()));

	// 更新封面边框颜色
	m_coverLabel->setStyleSheet(QString("QLabel{"
		"border-radius: 4px;"
		"background-color: #F5F5F5;"
		"border: 1px solid %1;"
		"}").arg(theme.borderColor.name()));
}

// 修改updatePendingUI和updateDownloadedUI函数，使用动态颜色
void DownloadCard::updatePendingUI()
{
	m_playIcon->setVisible(false);
	m_progressBar->setVisible(false);
	m_progressInfoLabel->setVisible(false);
	m_speedLabel->setVisible(false);

	// 显示时间和发布者信息
	m_timeLabel->setVisible(true);
	m_publisherLabel->setVisible(true);
}

void DownloadCard::updateDownloadingUI()
{
	// 更新进度信息
	m_progressBar->setValue(m_model->progress());
	m_speedLabel->setText(m_model->formattedDownloadSpeed());

	// 更新进度信息标签（已下载/总共）
	QString downloadedSize = DownloadTaskInfo::formatFileSize(m_model->downloadedSize());
	QString totalSize = DownloadTaskInfo::formatFileSize(m_model->downloadSize());
	m_progressInfoLabel->setText(QString("%1/%2").arg(downloadedSize).arg(totalSize));

	m_playIcon->setVisible(false);
	m_progressBar->setVisible(true);
	m_progressInfoLabel->setVisible(true);
	m_speedLabel->setVisible(true);

	// 隐藏时间和发布者信息
	m_timeLabel->setVisible(false);
	m_publisherLabel->setVisible(false);
}

void DownloadCard::updateDownloadedUI()
{
	m_playIcon->setVisible(true);
	m_progressBar->setVisible(false);
	m_progressInfoLabel->setVisible(false);
	m_speedLabel->setVisible(false);

	// 显示时间信息（改为下载完成时间），隐藏发布者信息
	m_timeLabel->setVisible(true);
	m_publisherLabel->setVisible(false);

	// 更新时间标签显示下载完成时间
	// 这里假设模型有下载完成时间，如果没有需要添加
	 //m_timeLabel->setText(QString("下载完成: %1").arg(m_model->formattedDownloadTime()));
}