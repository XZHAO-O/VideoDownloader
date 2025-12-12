#include "DownloadCard.h"

#include <QLabel>

#include "SvgButton.h"
#include "AntButton.h"
#include "AntCellWidget.h"
#include "DesignSystem.h"
#include "MaterialProgressBar.h"
#include "SingleLevelComboBox.h"
#include "StringUtil.h"
#include "Instrumentor.h"

DownloadCard::DownloadCard(DownloadCardState downloadCardState, QWidget* parent)
	: QWidget(parent)
	, m_currentState(downloadCardState)
{
	BENCHMARKING_FUNCTION();

	setObjectName("DownloadCard");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setMinimumHeight(160);
	setAttribute(Qt::WA_Hover, true);

	initUI();
	initConnections();
	updateTextColors();
	refreshUI();
	// 添加主题变化监听
	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]() {
		updateTextColors();
		update();
		});
}

DownloadCard::~DownloadCard()
{
	clearUI();
}

void DownloadCard::updateFromTaskInfo(QSharedPointer<DownloadTaskInfo> taskInfo)
{
	BENCHMARKING_FUNCTION();

	// 更新基本信息
	updateTitle(taskInfo->videoInfo.title);

	qint64 videoSize = 0;
	if (taskInfo->videoStreamInfo.contains(taskInfo->selectedVideoQuality)) {
		videoSize = taskInfo->videoStreamInfo[taskInfo->selectedVideoQuality].fileSize;
	}

	qint64 audioSize = 0;
	if (taskInfo->audioStreamInfo.contains(taskInfo->selectedAudioQuality)) {
		audioSize = taskInfo->audioStreamInfo[taskInfo->selectedAudioQuality].fileSize;
	}

	updateFileSizes(videoSize, audioSize);

	// 根据状态更新特定UI
	switch (m_currentState) {
	case DownloadCardState::Pending:
		updateTimeInfo(QString("%1 · %2")
			.arg(StringUtil::formatDateTime(QDateTime::fromSecsSinceEpoch(taskInfo->videoInfo.publishTime.toLongLong())))
			.arg(taskInfo->videoInfo.duration));
		updatePublisher(taskInfo->videoInfo.author);

		// 更新质量选项
		updateQualityOptions(taskInfo->videoStreamInfo.keys(), taskInfo->audioStreamInfo.keys());
		updateSelectedQuality(taskInfo->selectedVideoQuality, taskInfo->selectedAudioQuality);
		break;

	case DownloadCardState::Downloading:
		updateProgress(taskInfo->progressInfo);
		break;

	case DownloadCardState::Downloaded:
		updateTimeInfo(QString(tr("下载完成: %1")).arg(StringUtil::formatDateTime(taskInfo->endTime)));
		updateDownloadedFile(taskInfo->downloadFilePath);
		break;

	case DownloadCardState::Error:
		// 错误状态已由状态更新处理
		break;
	}

	// 更新封面
	updateCover(taskInfo->videoInfo.cover);
}

void DownloadCard::updateProgress(const ProgressInfo& progress)
{
	if (m_currentState != DownloadCardState::Downloading) return;

	if (m_progressBar) {
		m_progressBar->setValue(progress.progress);
	}
	if (m_speedLabel) {
		m_speedLabel->setText(progress.downloadSpeed);
	}
	if (m_progressInfoLabel) {
		m_progressInfoLabel->setText(progress.text);
	}
}

void DownloadCard::updateDownloadedFile(const QString& filePath)
{
	Q_UNUSED(filePath);
	// 已下载状态可以显示文件路径，但当前UI不需要
}

void DownloadCard::updateQualityOptions(const QStringList& videoQualities, const QStringList& audioQualities)
{
	if (m_currentState != DownloadCardState::Pending) return;

	setVideoQualityOptions(videoQualities);
	setAudioQualityOptions(audioQualities);
}

void DownloadCard::updateSelectedQuality(const QString& videoQuality, const QString& audioQuality)
{
	if (m_currentState != DownloadCardState::Pending) return;

	setCurrentVideoQuality(videoQuality);
	setCurrentAudioQuality(audioQuality);
}

void DownloadCard::updateCover(const QByteArray& coverData)
{
	if (!coverData.isEmpty() && m_coverLabel) {
		QPixmap pixmap;
		if (pixmap.loadFromData(coverData)) {
			m_coverLabel->setPixmap(pixmap.scaled(140, 105, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
			m_isCoverLoaded = true;
		}
	}
}

void DownloadCard::updateTitle(const QString& title)
{
	if (m_titleCell && m_titleCell->getBtn()) {
		m_titleCell->getBtn()->setText(title);
	}
}

void DownloadCard::updateFileSizes(qint64 videoSize, qint64 audioSize)
{
	if (m_sizeLabel) {
		m_sizeLabel->setText(QString(tr("视频: %1  音频: %2"))
			.arg(StringUtil::formatFileSize(videoSize))
			.arg(StringUtil::formatFileSize(audioSize)));
	}
}

void DownloadCard::updateTimeInfo(const QString& timeInfo)
{
	if (m_timeLabel) {
		m_timeLabel->setText(timeInfo);
	}
}

void DownloadCard::updatePublisher(const QString& publisher)
{
	if (m_publisherLabel) {
		m_publisherLabel->setText(publisher);
	}
}

void DownloadCard::setVideoQualityOptions(const QStringList& qualities)
{
	m_videoQualityCombo->disconnect(this);
	m_videoQualityCombo->setItemTextList(qualities);
	connect(m_videoQualityCombo, &SingleLevelComboBox::currentTextChanged, this, &DownloadCard::videoQualityChanged);
}

void DownloadCard::setAudioQualityOptions(const QStringList& qualities)
{
	m_audioQualityCombo->disconnect(this);
	m_audioQualityCombo->setItemTextList(qualities);
	connect(m_audioQualityCombo, &SingleLevelComboBox::currentTextChanged, this, &DownloadCard::audioQualityChanged);
}

void DownloadCard::setCurrentVideoQuality(const QString& quality)
{
	if (m_videoQualityCombo->itemTextList().contains(quality))
	{
		m_videoQualityCombo->setCurrentText(quality);
	}
}

void DownloadCard::setCurrentAudioQuality(const QString& quality)
{
	if (m_audioQualityCombo->itemTextList().contains(quality))
	{
		m_audioQualityCombo->setCurrentText(quality);
	}
}

QString DownloadCard::currentVideoQuality() const
{
	return m_videoQualityCombo->currentText();
}

QString DownloadCard::currentAudioQuality() const
{
	return m_audioQualityCombo->currentText();
}

QSize DownloadCard::sizeHint() const
{
	return QSize(680, 160);
}

QSize DownloadCard::minimumSizeHint() const
{
	return QSize(400, 160);
}

void DownloadCard::refreshUI()
{
	updateTextColors();
	update();
}

bool DownloadCard::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_coverContainer || watched == m_coverLabel) {
		if (event->type() == QEvent::Enter) {
			m_coverHovered = true;
			updatePlayIconVisibility(true);
			return true;
		}
		else if (event->type() == QEvent::Leave) {
			m_coverHovered = false;
			updatePlayIconVisibility(false);
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void DownloadCard::setupPlayIcon()
{
	if (!m_playIcon && m_coverContainer) {
		m_playIcon = new QLabel(m_coverContainer);
		m_playIcon->setFixedSize(40, 40);
		m_playIcon->setStyleSheet("QLabel{"
			"background-color: rgba(0, 0, 0, 0.6);"
			"border-radius: 20px;"
			"}");
		m_playIcon->setAlignment(Qt::AlignCenter);
		m_playIcon->setPixmap(QPixmap(":/Imgs/play.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		m_playIcon->setVisible(false);

		m_playIcon->raise();
	}
}

void DownloadCard::updatePlayIconVisibility(bool visible)
{
	m_playIcon->setVisible(visible);
}

void DownloadCard::clearUI()
{
	// 清理所有子控件和布局
	if (m_mainLayout) {
		QLayoutItem* item;
		while ((item = m_mainLayout->takeAt(0)) != nullptr) {
			if (item->widget()) {
				item->widget()->deleteLater();
			}
			delete item;
		}
		delete m_mainLayout;
		m_mainLayout = nullptr;
	}

	// 重置所有指针
	m_coverLabel = nullptr;
	m_playIcon = nullptr;
	m_titleCell = nullptr;
	m_sizeLabel = nullptr;
	m_timeLabel = nullptr;
	m_publisherLabel = nullptr;
	m_coverContainer = nullptr;
	m_contentLayout = nullptr;
	m_headerLayout = nullptr;
	m_middleLayout = nullptr;
	m_bottomLayout = nullptr;
	m_actionLayout = nullptr;

	m_videoQualityCombo = nullptr;
	m_audioQualityCombo = nullptr;
	m_downloadBtn = nullptr;
	m_videoDownloadBtn = nullptr;
	m_audioDownloadBtn = nullptr;
	m_closeBtn = nullptr;

	m_progressBar = nullptr;
	m_speedLabel = nullptr;
	m_progressInfoLabel = nullptr;
	m_pauseBtn_downloading = nullptr;
	m_openFolderBtn_downloading = nullptr;
	m_deleteBtn_downloading = nullptr;

	m_openUrlBtn_downloaded = nullptr;
	m_openFolderBtn_downloaded = nullptr;
	m_deleteBtn_downloaded = nullptr;

	m_errorLabel = nullptr;
	m_retryBtn = nullptr;
	m_closeBtn_error = nullptr;

	m_isCoverLoaded = false;
	m_coverHovered = false;
}

void DownloadCard::initUI()
{
	BENCHMARKING_FUNCTION();

	// 根据状态初始化不同的UI
	switch (m_currentState)
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
	case DownloadCardState::Error:
		initErrorUI();
		break;
	}
}

void DownloadCard::initPendingUI()
{
	BENCHMARKING_FUNCTION();

	// 主布局
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setSpacing(12);
	m_mainLayout->setContentsMargins(12, 4, 12, 4);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);
	m_coverContainer->setAttribute(Qt::WA_Hover, true);
	m_coverContainer->installEventFilter(this);

	m_coverContainer->setLayout(new QVBoxLayout());
	m_coverContainer->layout()->setContentsMargins(0, 0, 0, 0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setScaledContents(true);
	m_coverLabel->setText(QString("<a href='preview' style='text-decoration:none; color:transparent;'> </a>"));
	m_coverLabel->setAttribute(Qt::WA_Hover, true);
	m_coverLabel->installEventFilter(this);

	m_coverContainer->layout()->addWidget(m_coverLabel);
	setupPlayIcon();

	if (m_playIcon) {
		int x = (228 - 40) / 2;
		int y = (128 - 40) / 2;
		m_playIcon->move(x, y);
	}

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 10);

	// 头部布局（标题 + 操作按钮）
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	m_titleCell = new AntCellWidget("", this);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	// 创建待下载状态按钮
	m_downloadBtn = new AntButton(tr("下载"), 8, this);
	m_downloadBtn->setButtonMode(AntButton::Outlined);
	m_downloadBtn->setFixedSize(70, 35);

	m_videoDownloadBtn = new AntButton(tr("视频"), 8, this);
	m_videoDownloadBtn->setButtonMode(AntButton::Outlined);
	m_videoDownloadBtn->setFixedSize(70, 35);

	m_audioDownloadBtn = new AntButton(tr("音频"), 8, this);
	m_audioDownloadBtn->setButtonMode(AntButton::Outlined);
	m_audioDownloadBtn->setFixedSize(70, 35);

	m_closeBtn = new SvgButton("x", this);
	m_closeBtn->setIconSize(SvgButton::Medium);
	m_closeBtn->setFixedSize(32, 32);

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

	QWidget* sizeWidget = new QWidget(this);
	QHBoxLayout* sizeLayout = new QHBoxLayout(sizeWidget);
	sizeLayout->setContentsMargins(0, 0, 0, 0);
	sizeLayout->setSpacing(0);

	m_sizeLabel = new QLabel("", sizeWidget);
	m_sizeLabel->setFixedWidth(200);
	m_sizeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_sizeLabel->setTextFormat(Qt::PlainText);
	m_sizeLabel->setWordWrap(false);

	sizeLayout->addWidget(m_sizeLabel);

	// 质量选择容器
	QWidget* qualityWidget = new QWidget(this);
	QHBoxLayout* qualityLayout = new QHBoxLayout(qualityWidget);
	qualityLayout->setSpacing(8);
	qualityLayout->setContentsMargins(0, 0, 0, 0);

	QStringList qualityList = { "4K" };
	m_videoQualityCombo = new SingleLevelComboBox(tr("画质"), qualityList, this);
	m_videoQualityCombo->setFixedSize(170, 35);

	QStringList audioQualityList = { "无损" };
	m_audioQualityCombo = new SingleLevelComboBox(tr("音质"), audioQualityList, this);
	m_audioQualityCombo->setFixedSize(170, 35);

	qualityLayout->addWidget(m_videoQualityCombo);
	qualityLayout->addWidget(m_audioQualityCombo);
	qualityLayout->addStretch();

	m_middleLayout->addWidget(sizeWidget);
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

	m_timeLabel = new QLabel("", this);
	timeLayout->addWidget(m_timeLabel);
	timeLayout->addStretch();

	// 第二行：发布者信息
	QHBoxLayout* publisherLayout = new QHBoxLayout();
	publisherLayout->setSpacing(12);
	publisherLayout->setContentsMargins(0, 0, 0, 0);

	m_publisherLabel = new QLabel("", this);
	publisherLayout->addWidget(m_publisherLabel);
	publisherLayout->addStretch();

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
	m_mainLayout->setContentsMargins(12, 4, 12, 4);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);
	m_coverContainer->setAttribute(Qt::WA_Hover, true);
	m_coverContainer->installEventFilter(this);

	m_coverContainer->setLayout(new QVBoxLayout());
	m_coverContainer->layout()->setContentsMargins(0, 0, 0, 0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setScaledContents(true);
	m_coverLabel->setAttribute(Qt::WA_Hover, true);
	m_coverLabel->installEventFilter(this);

	m_coverContainer->layout()->addWidget(m_coverLabel);
	setupPlayIcon();

	if (m_playIcon) {
		int x = (228 - 40) / 2;
		int y = (128 - 40) / 2;
		m_playIcon->move(x, y);
	}

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	m_titleCell = new AntCellWidget("", this);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	m_pauseBtn_downloading = new SvgButton("pause-circle", this);
	m_pauseBtn_downloading->setIconSize(SvgButton::Medium);
	m_pauseBtn_downloading->setFixedSize(32, 32);
	m_pauseBtn_downloading->setToolTip(tr("暂停"));

	m_openFolderBtn_downloading = new SvgButton("folder2", this);
	m_openFolderBtn_downloading->setIconSize(SvgButton::Medium);
	m_openFolderBtn_downloading->setFixedSize(32, 32);
	m_openFolderBtn_downloading->setToolTip(tr("打开文件夹"));

	m_deleteBtn_downloading = new SvgButton("trash", this);
	m_deleteBtn_downloading->setIconSize(SvgButton::Medium);
	m_deleteBtn_downloading->setFixedSize(32, 32);
	m_deleteBtn_downloading->setToolTip(tr("删除"));

	m_actionLayout->addWidget(m_pauseBtn_downloading);
	m_actionLayout->addWidget(m_openFolderBtn_downloading);
	m_actionLayout->addWidget(m_deleteBtn_downloading);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("", this);
	m_middleLayout->addWidget(m_sizeLabel);
	m_middleLayout->addStretch();

	// 底部布局 - 进度信息
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	QWidget* progressWidget = new QWidget(this);
	QVBoxLayout* progressLayout = new QVBoxLayout(progressWidget);
	progressLayout->setSpacing(4);
	progressLayout->setContentsMargins(0, 0, 0, 16);

	QHBoxLayout* progressInfoLayout = new QHBoxLayout();
	progressInfoLayout->setSpacing(8);
	progressInfoLayout->setContentsMargins(0, 0, 0, 2);

	m_progressInfoLabel = new QLabel("", this);
	m_speedLabel = new QLabel("", this);

	progressInfoLayout->addWidget(m_progressInfoLabel);
	progressInfoLayout->addStretch();
	progressInfoLayout->addWidget(m_speedLabel);

	m_progressBar = new MaterialProgressBar(this);
	m_progressBar->setFixedHeight(12);

	progressLayout->addLayout(progressInfoLayout);
	progressLayout->addWidget(m_progressBar);

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
	m_mainLayout->setContentsMargins(12, 4, 12, 4);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);
	m_coverContainer->setAttribute(Qt::WA_Hover, true);
	m_coverContainer->installEventFilter(this);

	m_coverContainer->setLayout(new QVBoxLayout());
	m_coverContainer->layout()->setContentsMargins(0, 0, 0, 0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setScaledContents(true);
	m_coverLabel->setText(QString("<a href='preview' style='text-decoration:none; color:transparent;'> </a>"));
	m_coverLabel->setAttribute(Qt::WA_Hover, true);
	m_coverLabel->installEventFilter(this);

	m_coverContainer->layout()->addWidget(m_coverLabel);
	setupPlayIcon();

	if (m_playIcon) {
		int x = (228 - 40) / 2;
		int y = (128 - 40) / 2;
		m_playIcon->move(x, y);
	}

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	m_titleCell = new AntCellWidget("", this);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	m_openUrlBtn_downloaded = new SvgButton("link-45deg", this);
	m_openUrlBtn_downloaded->setIconSize(SvgButton::Medium);
	m_openUrlBtn_downloaded->setFixedSize(32, 32);
	m_openUrlBtn_downloaded->setToolTip(tr("打开链接"));

	m_openFolderBtn_downloaded = new SvgButton("folder2", this);
	m_openFolderBtn_downloaded->setIconSize(SvgButton::Medium);
	m_openFolderBtn_downloaded->setFixedSize(32, 32);
	m_openFolderBtn_downloaded->setToolTip(tr("打开文件夹"));

	m_deleteBtn_downloaded = new SvgButton("trash", this);
	m_deleteBtn_downloaded->setIconSize(SvgButton::Medium);
	m_deleteBtn_downloaded->setFixedSize(32, 32);
	m_deleteBtn_downloaded->setToolTip(tr("删除"));

	m_actionLayout->addWidget(m_openUrlBtn_downloaded);
	m_actionLayout->addWidget(m_openFolderBtn_downloaded);
	m_actionLayout->addWidget(m_deleteBtn_downloaded);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("", this);
	m_middleLayout->addWidget(m_sizeLabel);
	m_middleLayout->addStretch();

	// 底部布局
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* timeLayout = new QHBoxLayout();
	timeLayout->setSpacing(12);
	timeLayout->setContentsMargins(0, 0, 0, 0);

	m_timeLabel = new QLabel("", this);
	timeLayout->addWidget(m_timeLabel);
	timeLayout->addStretch();

	m_bottomLayout->addLayout(timeLayout);

	// 组装内容布局
	m_contentLayout->addLayout(m_headerLayout);
	m_contentLayout->addLayout(m_middleLayout);
	m_contentLayout->addLayout(m_bottomLayout);

	// 组装主布局
	m_mainLayout->addWidget(m_coverContainer);
	m_mainLayout->addWidget(contentWidget, 1);
}

void DownloadCard::initErrorUI()
{
	BENCHMARKING_FUNCTION();

	// 主布局
	m_mainLayout = new QHBoxLayout(this);
	m_mainLayout->setSpacing(12);
	m_mainLayout->setContentsMargins(12, 4, 12, 4);

	// 封面容器
	m_coverContainer = new QWidget(this);
	m_coverContainer->setFixedSize(228, 128);
	m_coverContainer->setLayout(new QVBoxLayout());
	m_coverContainer->layout()->setContentsMargins(0, 0, 0, 0);

	m_coverLabel = new QLabel(m_coverContainer);
	m_coverLabel->setFixedSize(228, 128);
	m_coverLabel->setScaledContents(true);
	m_coverContainer->layout()->addWidget(m_coverLabel);

	// 内容区域
	QWidget* contentWidget = new QWidget(this);
	contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	m_contentLayout = new QVBoxLayout(contentWidget);
	m_contentLayout->setSpacing(8);
	m_contentLayout->setContentsMargins(0, 0, 0, 0);

	// 头部布局
	m_headerLayout = new QHBoxLayout();
	m_headerLayout->setSpacing(8);
	m_headerLayout->setContentsMargins(0, 0, 0, 0);

	m_titleCell = new AntCellWidget("", this);

	// 操作按钮容器
	QWidget* actionWidget = new QWidget(this);
	m_actionLayout = new QHBoxLayout(actionWidget);
	m_actionLayout->setSpacing(6);
	m_actionLayout->setContentsMargins(0, 0, 0, 0);

	m_retryBtn = new SvgButton("arrow-clockwise", this);
	m_retryBtn->setIconSize(SvgButton::Medium);
	m_retryBtn->setFixedSize(32, 32);
	m_retryBtn->setToolTip(tr("重试"));

	m_closeBtn_error = new SvgButton("x", this);
	m_closeBtn_error->setIconSize(SvgButton::Medium);
	m_closeBtn_error->setFixedSize(32, 32);
	m_closeBtn_error->setToolTip(tr("关闭"));

	m_actionLayout->addWidget(m_retryBtn);
	m_actionLayout->addWidget(m_closeBtn_error);
	m_actionLayout->addStretch();

	m_headerLayout->addWidget(m_titleCell, 1);
	m_headerLayout->addWidget(actionWidget);

	// 中间布局
	m_middleLayout = new QHBoxLayout();
	m_middleLayout->setSpacing(20);
	m_middleLayout->setContentsMargins(0, 0, 0, 0);

	m_errorLabel = new QLabel(tr("下载失败"), this);
	m_middleLayout->addWidget(m_errorLabel);
	m_middleLayout->addStretch();

	// 底部布局
	m_bottomLayout = new QVBoxLayout();
	m_bottomLayout->setSpacing(4);
	m_bottomLayout->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* infoLayout = new QHBoxLayout();
	infoLayout->setSpacing(12);
	infoLayout->setContentsMargins(0, 0, 0, 0);

	m_sizeLabel = new QLabel("", this);
	infoLayout->addWidget(m_sizeLabel);
	infoLayout->addStretch();

	m_bottomLayout->addLayout(infoLayout);

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
	if (m_coverLabel)
	{
		connect(m_coverLabel, &QLabel::linkActivated, this, &DownloadCard::onCoverClicked);
	}

	if (m_titleCell && m_titleCell->getBtn())
	{
		connect(m_titleCell->getBtn(), &QPushButton::clicked, this, &DownloadCard::onTitleClicked);
	}

	// 根据状态初始化特定连接
	switch (m_currentState)
	{
	case DownloadCardState::Pending:
		if (m_videoQualityCombo)
		{
			connect(m_videoQualityCombo, &SingleLevelComboBox::currentTextChanged, this, &DownloadCard::videoQualityChanged);
		}

		if (m_audioQualityCombo)
		{
			connect(m_audioQualityCombo, &SingleLevelComboBox::currentTextChanged, this, &DownloadCard::audioQualityChanged);
		}

		if (m_downloadBtn)
		{
			connect(m_downloadBtn, &AntButton::clicked, this, &DownloadCard::downloadClicked);
		}

		if (m_videoDownloadBtn)
		{
			connect(m_videoDownloadBtn, &AntButton::clicked, this, &DownloadCard::videoDownloadClicked);
		}

		if (m_audioDownloadBtn)
		{
			connect(m_audioDownloadBtn, &AntButton::clicked, this, &DownloadCard::audioDownloadClicked);
		}

		if (m_closeBtn)
		{
			connect(m_closeBtn, &SvgButton::clicked, this, &DownloadCard::deleteClicked);
		}
		break;

	case DownloadCardState::Downloading:
		if (m_pauseBtn_downloading)
		{
			connect(m_pauseBtn_downloading, &SvgButton::clicked, this, &DownloadCard::pauseClicked);
		}

		if (m_openFolderBtn_downloading)
		{
			connect(m_openFolderBtn_downloading, &SvgButton::clicked, this, &DownloadCard::openFolderClicked);
		}

		if (m_deleteBtn_downloading)
		{
			connect(m_deleteBtn_downloading, &SvgButton::clicked, this, &DownloadCard::deleteClicked);
		}
		break;

	case DownloadCardState::Downloaded:
		if (m_openUrlBtn_downloaded)
		{
			connect(m_openUrlBtn_downloaded, &SvgButton::clicked, this, &DownloadCard::openUrlClicked);
		}

		if (m_openFolderBtn_downloaded)
		{
			connect(m_openFolderBtn_downloaded, &SvgButton::clicked, this, &DownloadCard::openFolderClicked);
		}

		if (m_deleteBtn_downloaded)
		{
			connect(m_deleteBtn_downloaded, &SvgButton::clicked, this, &DownloadCard::deleteClicked);
		}
		break;

	case DownloadCardState::Error:
		if (m_retryBtn)
		{
			connect(m_retryBtn, &SvgButton::clicked, this, &DownloadCard::retryClicked);
		}

		if (m_closeBtn_error)
		{
			connect(m_closeBtn_error, &SvgButton::clicked, this, &DownloadCard::deleteClicked);
		}
		break;
	}
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
			"}").arg(theme.secondaryTextColor.name()));

		m_publisherLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.secondaryTextColor.name()));
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
		m_timeLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.secondaryTextColor.name()));
		break;

	case DownloadCardState::Error:
		m_errorLabel->setStyleSheet(QString("QLabel{"
			"font-size: 11px;"
			"color: %1;"
			"}").arg(theme.secondaryTextColor.name()));
		break;
	}
}

void DownloadCard::mousePressEvent(QMouseEvent* event)
{
	QWidget::mousePressEvent(event);
}

void DownloadCard::enterEvent(QEnterEvent* event)
{
	QWidget::enterEvent(event);
}

void DownloadCard::leaveEvent(QEvent* event)
{
	QWidget::leaveEvent(event);
}

void DownloadCard::paintEvent(QPaintEvent* event)
{
	BENCHMARKING_FUNCTION();

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	QRect bgRect = rect();
	auto theme = DesignSystem::instance()->currentTheme();
	QColor bgColor = theme.cardBackgroundColor;

	painter.setBrush(bgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(bgRect, 8, 8);

	// 绘制边框
	QPen borderPen = QPen(theme.borderColor);
	borderPen.setWidth(1);
	painter.setPen(borderPen);
	painter.setBrush(Qt::NoBrush);
	painter.drawRoundedRect(bgRect.adjusted(1, 1, -1, -1), 8, 8);

	QWidget::paintEvent(event);
}

void DownloadCard::onCoverClicked()
{
	if (m_currentState == DownloadCardState::Downloaded)
	{
		emit previewClicked();
	}
}

void DownloadCard::onTitleClicked()
{
	if (m_currentState == DownloadCardState::Downloaded) {
		onCoverClicked();
	}
	else {
		emit openUrlClicked();
	}
}