#include "SettingsPage.h"

#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QStandardPaths>
#include <QButtonGroup>
#include <QDesktopServices>

#include "AntButton.h"
#include "AntToggleButton.h"
#include "AntRadioButton.h"
#include "AntComboBox.h"
#include "MaterialTabWidget.h"
#include "logger.h"
#include "ApplicationController.h"
#include "ConfigManager.h"
#include "Instrumentor.h"

SettingsPage::SettingsPage(QSharedPointer<ApplicationController> appController, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
	, m_configManager(m_appController->getConfigManager())
{
	BENCHMARKING_FUNCTION();
	setObjectName("SettingsPage");
	setupUI();
	setupConnections();
	loadCurrentSettings();
}

SettingsPage::~SettingsPage()
{
}

void SettingsPage::setupUI()
{
	BENCHMARKING_FUNCTION();
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(0);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);

	// 创建选项卡
	m_tabWidget = new MaterialTabWidget(this);
	m_tabWidget->getLayout()->setContentsMargins(15, 0, 5, 0);

	// 设置各个选项卡
	setupGeneralSettings();
	setupDownloadSettings();
	setupNetworkSettings();
	setupAdvancedSettings();

	m_mainLayout->addWidget(m_tabWidget);
}

void SettingsPage::setupGeneralSettings()
{
	m_generalTab = new QWidget(this);
	m_generalLayout = new QVBoxLayout(m_generalTab);
	m_generalLayout->setSpacing(20);
	m_generalLayout->setContentsMargins(20, 20, 20, 20);

	// 开机自启动
	QHBoxLayout* autoStartLayout = new QHBoxLayout();
	QLabel* autoStartLabel = new QLabel(tr("开机自启动"), m_generalTab);
	m_autoStartToggle = new AntToggleButton(QSize(57, 26), m_generalTab);
	m_autoStartToggle->setShowText(true);
	autoStartLayout->addWidget(autoStartLabel);
	autoStartLayout->addWidget(m_autoStartToggle);
	autoStartLayout->addStretch();
	m_generalLayout->addLayout(autoStartLayout);

	// 自动检查更新
	QHBoxLayout* checkUpdatesLayout = new QHBoxLayout();
	QLabel* checkUpdatesLabel = new QLabel(tr("自动检查更新"), m_generalTab);
	m_checkUpdatesToggle = new AntToggleButton(QSize(57, 26), m_generalTab);
	m_checkUpdatesToggle->setShowText(true);
	checkUpdatesLayout->addWidget(checkUpdatesLabel);
	checkUpdatesLayout->addWidget(m_checkUpdatesToggle);
	checkUpdatesLayout->addStretch();
	m_generalLayout->addLayout(checkUpdatesLayout);

	// 退出行为
	m_exitBehaviorLayout = new QHBoxLayout();
	m_exitBehaviorLabel = new QLabel(tr("退出行为"), m_generalTab);
	m_exitProgramRadio = new AntRadioButton(m_generalTab);
	m_exitProgramRadio->setText(tr("退出程序"));
	m_minimizeToTrayRadio = new AntRadioButton(m_generalTab);
	m_minimizeToTrayRadio->setText(tr("最小化到系统托盘"));

	m_exitBehaviorLayout->addWidget(m_exitBehaviorLabel);
	m_exitBehaviorLayout->addWidget(m_exitProgramRadio);
	m_exitBehaviorLayout->addWidget(m_minimizeToTrayRadio);
	m_exitBehaviorLayout->addStretch();
	m_generalLayout->addLayout(m_exitBehaviorLayout);

	// 语言设置
	m_languageLayout = new QHBoxLayout();
	m_languageLabel = new QLabel(tr("界面语言"), m_generalTab);

	// 创建语言列表
	QStringList languages;
	languages << tr("简体中文") << tr("English") << tr("日本語");

	// 使用当前语言作为默认显示文本
	QString currentLanguage = m_configManager->getValue("ui/language").toString();
	m_languageCombo = new AntComboBox(currentLanguage, languages, m_generalTab);
	m_languageCombo->setFixedSize(185, 48);

	m_languageLayout->addWidget(m_languageLabel);
	m_languageLayout->addWidget(m_languageCombo);
	m_languageLayout->addStretch();
	m_generalLayout->addLayout(m_languageLayout);

	// 重置按钮
	QHBoxLayout* resetLayout = new QHBoxLayout();
	m_resetButton = new AntButton(tr("重置"), 11, m_generalTab);
	m_resetButton->setButtonColor(DesignSystem::instance()->dangerColor());
	m_resetButton->setFixedSize(120, 40);
	m_resetButton->setToolTip(tr("重置所有设置为默认状态"));

	resetLayout->addWidget(m_resetButton);
	resetLayout->addStretch();
	m_generalLayout->addLayout(resetLayout);

	m_generalLayout->addStretch();
	m_tabWidget->addTab(m_generalTab, tr("常规"));
}

void SettingsPage::setupDownloadSettings()
{
	m_downloadTab = new QWidget(this);
	m_downloadLayout = new QVBoxLayout(m_downloadTab);
	m_downloadLayout->setSpacing(20);
	m_downloadLayout->setContentsMargins(20, 20, 20, 20);

	// 下载路径
	m_downloadPathLayout = new QHBoxLayout();
	m_downloadPathLabel = new QLabel(tr("下载路径"), m_downloadTab);
	m_downloadPathInput = new QLineEdit(m_downloadTab);
	m_browsePathButton = new AntButton(tr("浏览"), 11, m_downloadTab);
	m_browsePathButton->setFixedSize(80, 40);

	m_downloadPathLayout->addWidget(m_downloadPathLabel);
	m_downloadPathLayout->addWidget(m_downloadPathInput);
	m_downloadPathLayout->addWidget(m_browsePathButton);
	m_downloadPathLayout->addStretch();
	m_downloadLayout->addLayout(m_downloadPathLayout);

	// 视频质量
	m_videoQualityLayout = new QHBoxLayout();
	m_videoQualityLabel = new QLabel("画质", m_downloadTab);
	m_videoQualityHighest = new AntRadioButton(m_downloadTab);
	m_videoQualityHighest->setText("最高质量");
	m_videoQuality1080P = new AntRadioButton(m_downloadTab);
	m_videoQuality1080P->setText("1080P");
	m_videoQuality720P = new AntRadioButton(m_downloadTab);
	m_videoQuality720P->setText("720P");
	m_videoQuality480P = new AntRadioButton(m_downloadTab);
	m_videoQuality480P->setText("480P");
	m_videoQuality360P = new AntRadioButton(m_downloadTab);
	m_videoQuality360P->setText("360P");

	// 创建视频质量按钮组
	QButtonGroup* videoQualityGroup = new QButtonGroup(this);
	videoQualityGroup->addButton(m_videoQualityHighest);
	videoQualityGroup->addButton(m_videoQuality1080P);
	videoQualityGroup->addButton(m_videoQuality720P);
	videoQualityGroup->addButton(m_videoQuality480P);
	videoQualityGroup->addButton(m_videoQuality360P);

	m_videoQualityLayout->addWidget(m_videoQualityLabel);
	m_videoQualityLayout->addWidget(m_videoQualityHighest);
	m_videoQualityLayout->addWidget(m_videoQuality1080P);
	m_videoQualityLayout->addWidget(m_videoQuality720P);
	m_videoQualityLayout->addWidget(m_videoQuality480P);
	m_videoQualityLayout->addWidget(m_videoQuality360P);
	m_videoQualityLayout->addStretch();
	m_downloadLayout->addLayout(m_videoQualityLayout);

	// 音频质量
	m_audioQualityLayout = new QHBoxLayout();
	m_audioQualityLabel = new QLabel("音质", m_downloadTab);
	m_audioQualityHighest = new AntRadioButton(m_downloadTab);
	m_audioQualityHighest->setText("最高质量");
	m_audioQuality320k = new AntRadioButton(m_downloadTab);
	m_audioQuality320k->setText("320kbps");
	m_audioQuality256k = new AntRadioButton(m_downloadTab);
	m_audioQuality256k->setText("256kbps");
	m_audioQuality192k = new AntRadioButton(m_downloadTab);
	m_audioQuality192k->setText("192kbps");
	m_audioQuality128k = new AntRadioButton(m_downloadTab);
	m_audioQuality128k->setText("128kbps");

	// 创建音频质量按钮组
	QButtonGroup* audioQualityGroup = new QButtonGroup(this);
	audioQualityGroup->addButton(m_audioQualityHighest);
	audioQualityGroup->addButton(m_audioQuality320k);
	audioQualityGroup->addButton(m_audioQuality256k);
	audioQualityGroup->addButton(m_audioQuality192k);
	audioQualityGroup->addButton(m_audioQuality128k);

	m_audioQualityLayout->addWidget(m_audioQualityLabel);
	m_audioQualityLayout->addWidget(m_audioQualityHighest);
	m_audioQualityLayout->addWidget(m_audioQuality320k);
	m_audioQualityLayout->addWidget(m_audioQuality256k);
	m_audioQualityLayout->addWidget(m_audioQuality192k);
	m_audioQualityLayout->addWidget(m_audioQuality128k);
	m_audioQualityLayout->addStretch();
	m_downloadLayout->addLayout(m_audioQualityLayout);

	// 下载格式
	m_downloadFormatLayout = new QHBoxLayout();
	m_downloadFormatLabel = new QLabel(tr("下载格式"), m_downloadTab);
	m_formatMerge = new AntRadioButton(m_downloadTab);
	m_formatMerge->setText(tr("视频+音频(合并)"));
	m_formatVideoOnly = new AntRadioButton(m_downloadTab);
	m_formatVideoOnly->setText(tr("仅视频"));
	m_formatAudioOnly = new AntRadioButton(m_downloadTab);
	m_formatAudioOnly->setText(tr("仅音频"));
	m_formatSeparate = new AntRadioButton(m_downloadTab);
	m_formatSeparate->setText(tr("视频+音频(分离)"));

	// 创建下载格式按钮组
	QButtonGroup* formatGroup = new QButtonGroup(this);
	formatGroup->addButton(m_formatMerge);
	formatGroup->addButton(m_formatVideoOnly);
	formatGroup->addButton(m_formatAudioOnly);
	formatGroup->addButton(m_formatSeparate);

	m_downloadFormatLayout->addWidget(m_downloadFormatLabel);
	m_downloadFormatLayout->addWidget(m_formatMerge);
	m_downloadFormatLayout->addWidget(m_formatVideoOnly);
	m_downloadFormatLayout->addWidget(m_formatAudioOnly);
	m_downloadFormatLayout->addWidget(m_formatSeparate);
	m_downloadFormatLayout->addStretch();
	m_downloadLayout->addLayout(m_downloadFormatLayout);

	// 同时下载数量
	m_concurrentDownloadsLayout = new QHBoxLayout();
	m_concurrentDownloadsLabel = new QLabel(tr("同时下载"), m_downloadTab);
	m_concurrent1 = new AntRadioButton(m_downloadTab);
	m_concurrent1->setText("1");
	m_concurrent2 = new AntRadioButton(m_downloadTab);
	m_concurrent2->setText("2");
	m_concurrent3 = new AntRadioButton(m_downloadTab);
	m_concurrent3->setText("3");
	m_concurrent4 = new AntRadioButton(m_downloadTab);
	m_concurrent4->setText("4");
	m_concurrent5 = new AntRadioButton(m_downloadTab);
	m_concurrent5->setText("5");

	// 创建同时下载数量按钮组
	QButtonGroup* concurrentGroup = new QButtonGroup(this);
	concurrentGroup->addButton(m_concurrent1);
	concurrentGroup->addButton(m_concurrent2);
	concurrentGroup->addButton(m_concurrent3);
	concurrentGroup->addButton(m_concurrent4);
	concurrentGroup->addButton(m_concurrent5);

	m_concurrentDownloadsLayout->addWidget(m_concurrentDownloadsLabel);
	m_concurrentDownloadsLayout->addWidget(m_concurrent1);
	m_concurrentDownloadsLayout->addWidget(m_concurrent2);
	m_concurrentDownloadsLayout->addWidget(m_concurrent3);
	m_concurrentDownloadsLayout->addWidget(m_concurrent4);
	m_concurrentDownloadsLayout->addWidget(m_concurrent5);
	m_concurrentDownloadsLayout->addStretch();
	m_downloadLayout->addLayout(m_concurrentDownloadsLayout);

	m_downloadLayout->addStretch();
	m_tabWidget->addTab(m_downloadTab, tr("下载"));
}

void SettingsPage::setupNetworkSettings()
{
	m_networkTab = new QWidget(this);
	m_networkLayout = new QVBoxLayout(m_networkTab);
	m_networkLayout->setSpacing(20);
	m_networkLayout->setContentsMargins(20, 20, 20, 20);

	// 代理设置 - 启用代理
	m_proxyEnabledLayout = new QHBoxLayout();
	m_proxyEnabledLabel = new QLabel(tr("启用代理"), m_networkTab);
	m_proxyEnabledToggle = new AntToggleButton(QSize(57, 26), m_networkTab);
	m_proxyEnabledToggle->setShowText(true);

	m_proxyEnabledLayout->addWidget(m_proxyEnabledLabel);
	m_proxyEnabledLayout->addWidget(m_proxyEnabledToggle);
	m_proxyEnabledLayout->addStretch();
	m_networkLayout->addLayout(m_proxyEnabledLayout);

	// 代理类型
	m_proxyTypeLayout = new QHBoxLayout();
	m_proxyTypeLabel = new QLabel(tr("代理类型"), m_networkTab);

	QStringList proxyTypes;
	proxyTypes << "HTTP" << "SOCKS5" << "HTTPS";
	m_proxyTypeCombo = new AntComboBox("HTTP", proxyTypes, m_networkTab);
	m_proxyTypeCombo->setFixedSize(185, 48);

	m_proxyTypeLayout->addWidget(m_proxyTypeLabel);
	m_proxyTypeLayout->addWidget(m_proxyTypeCombo);
	m_proxyTypeLayout->addStretch();
	m_networkLayout->addLayout(m_proxyTypeLayout);

	// 代理主机
	m_proxyHostLayout = new QHBoxLayout();
	m_proxyHostLabel = new QLabel(tr("代理主机"), m_networkTab);
	m_proxyHostInput = new QLineEdit(m_networkTab);
	m_proxyHostInput->setPlaceholderText("proxy.example.com");

	m_proxyHostLayout->addWidget(m_proxyHostLabel);
	m_proxyHostLayout->addWidget(m_proxyHostInput);
	m_proxyHostLayout->addStretch();
	m_networkLayout->addLayout(m_proxyHostLayout);

	// 代理端口
	m_proxyPortLayout = new QHBoxLayout();
	m_proxyPortLabel = new QLabel(tr("代理端口"), m_networkTab);
	m_proxyPortInput = new QLineEdit(m_networkTab);
	m_proxyPortInput->setPlaceholderText("8080");

	m_proxyPortLayout->addWidget(m_proxyPortLabel);
	m_proxyPortLayout->addWidget(m_proxyPortInput);
	m_proxyPortLayout->addStretch();
	m_networkLayout->addLayout(m_proxyPortLayout);

	// 代理用户名
	m_proxyUserLayout = new QHBoxLayout();
	m_proxyUserLabel = new QLabel(tr("代理用户名"), m_networkTab);
	m_proxyUserInput = new QLineEdit(m_networkTab);
	m_proxyUserInput->setPlaceholderText(tr("用户名"));

	m_proxyUserLayout->addWidget(m_proxyUserLabel);
	m_proxyUserLayout->addWidget(m_proxyUserInput);
	m_proxyUserLayout->addStretch();
	m_networkLayout->addLayout(m_proxyUserLayout);

	// 代理密码
	m_proxyPassLayout = new QHBoxLayout();
	m_proxyPassLabel = new QLabel(tr("代理密码"), m_networkTab);
	m_proxyPassInput = new QLineEdit(m_networkTab);
	m_proxyPassInput->setPlaceholderText(tr("密码"));
	m_proxyPassInput->setEchoMode(QLineEdit::Password);

	m_proxyPassLayout->addWidget(m_proxyPassLabel);
	m_proxyPassLayout->addWidget(m_proxyPassInput);
	m_proxyPassLayout->addStretch();
	m_networkLayout->addLayout(m_proxyPassLayout);

	// 其他网络设置
	// 超时时间
	m_timeoutLayout = new QHBoxLayout();
	m_timeoutLabel = new QLabel(tr("超时时间"), m_networkTab);
	m_timeoutInput = new QLineEdit(m_networkTab);
	m_timeoutInput->setPlaceholderText(tr("毫秒"));
	m_timeoutInput->setFixedWidth(100);

	m_timeoutLayout->addWidget(m_timeoutLabel);
	m_timeoutLayout->addWidget(m_timeoutInput);
	m_timeoutLayout->addStretch();
	m_networkLayout->addLayout(m_timeoutLayout);

	// 重试次数
	m_retryCountLayout = new QHBoxLayout();
	m_retryCountLabel = new QLabel(tr("重试次数"), m_networkTab);
	m_retryCountInput = new QLineEdit(m_networkTab);
	//m_retryCountInput->setPlaceholderText(tr("次"));
	m_retryCountInput->setFixedWidth(100);

	m_retryCountLayout->addWidget(m_retryCountLabel);
	m_retryCountLayout->addWidget(m_retryCountInput);
	m_retryCountLayout->addStretch();
	m_networkLayout->addLayout(m_retryCountLayout);

	// 用户代理
	m_userAgentLayout = new QHBoxLayout();
	m_userAgentLabel = new QLabel(tr("用户代理"), m_networkTab);
	m_userAgentInput = new QLineEdit(m_networkTab);
	m_userAgentInput->setPlaceholderText("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

	m_userAgentLayout->addWidget(m_userAgentLabel);
	m_userAgentLayout->addWidget(m_userAgentInput);
	m_userAgentLayout->addStretch();
	m_networkLayout->addLayout(m_userAgentLayout);

	m_networkLayout->addStretch();
	m_tabWidget->addTab(m_networkTab, tr("网络"));
}

void SettingsPage::setupAdvancedSettings()
{
	m_advancedTab = new QWidget(this);
	m_advancedLayout = new QVBoxLayout(m_advancedTab);
	m_advancedLayout->setSpacing(20);
	m_advancedLayout->setContentsMargins(20, 20, 20, 20);

	// 日志路径
	m_logPathLayout = new QHBoxLayout();
	m_logPathLabel = new QLabel(tr("日志路径"), m_advancedTab);
	m_logPathInput = new QLineEdit(m_advancedTab);
	m_browseLogPathButton = new AntButton(tr("浏览"), 11, m_advancedTab);
	m_browseLogPathButton->setFixedSize(80, 40);

	m_logPathLayout->addWidget(m_logPathLabel);
	m_logPathLayout->addWidget(m_logPathInput);
	m_logPathLayout->addWidget(m_browseLogPathButton);
	m_logPathLayout->addStretch();
	m_advancedLayout->addLayout(m_logPathLayout);

	// 日志操作按钮
	m_logButtonsLayout = new QHBoxLayout();
	m_viewLogsButton = new AntButton(tr("查看日志"), 11, m_advancedTab);
	m_viewLogsButton->setFixedSize(120, 40);
	m_clearLogsButton = new AntButton(tr("清空日志"), 11, m_advancedTab);
	m_clearLogsButton->setFixedSize(120, 40);

	m_logButtonsLayout->addWidget(m_viewLogsButton);
	m_logButtonsLayout->addWidget(m_clearLogsButton);
	m_logButtonsLayout->addStretch();
	m_advancedLayout->addLayout(m_logButtonsLayout);

	m_advancedLayout->addStretch();
	m_tabWidget->addTab(m_advancedTab, tr("高级"));
}

void SettingsPage::setupConnections()
{
	BENCHMARKING_FUNCTION();
	// 重置按钮连接
	connect(m_resetButton, &AntButton::clicked, this, [this]() {
		emit showResetDialog(tr("重置设置"), tr("确定要重置设置为默认状态吗？"));
		});

	// 常规设置自动保存
	connect(m_autoStartToggle, &AntToggleButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_checkUpdatesToggle, &AntToggleButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_exitProgramRadio, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_minimizeToTrayRadio, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_languageCombo, &AntComboBox::currentTextChanged, this, &SettingsPage::autoSaveSettings);

	// 下载设置自动保存
	connect(m_downloadPathInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_videoQualityHighest, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_videoQuality1080P, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_videoQuality720P, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_videoQuality480P, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_videoQuality360P, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_audioQualityHighest, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_audioQuality320k, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_audioQuality256k, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_audioQuality192k, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_audioQuality128k, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_formatMerge, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_formatVideoOnly, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_formatAudioOnly, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_formatSeparate, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_concurrent1, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_concurrent2, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_concurrent3, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_concurrent4, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_concurrent5, &AntRadioButton::toggled, this, &SettingsPage::autoSaveSettings);

	// 网络设置自动保存
	connect(m_proxyEnabledToggle, &AntToggleButton::toggled, this, &SettingsPage::autoSaveSettings);
	connect(m_proxyTypeCombo, &AntComboBox::currentTextChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_proxyHostInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_proxyPortInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_proxyUserInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_proxyPassInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_timeoutInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_retryCountInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);
	connect(m_userAgentInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);

	// 高级设置自动保存
	connect(m_logPathInput, &QLineEdit::textChanged, this, &SettingsPage::autoSaveSettings);

	// 其他连接
	connect(m_browsePathButton, &AntButton::clicked, this, &SettingsPage::onDownloadPathBrowse);
	connect(m_browseLogPathButton, &AntButton::clicked, this, [this]() {
		QString path = QFileDialog::getExistingDirectory(this, tr("选择日志目录"), m_logPathInput->text());
		if (!path.isEmpty()) {
			m_logPathInput->setText(path);
		}
		});

	connect(m_proxyEnabledToggle, &AntToggleButton::toggled, this, &SettingsPage::onProxySettingsChanged);

	connect(m_viewLogsButton, &AntButton::clicked, this, [this]() {
		QString logDir = m_configManager->getValue("log/path").toString();
		QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
		});

	connect(m_clearLogsButton, &AntButton::clicked, this, [this]() {
		emit showLogClearDialog(tr("清空日志"), tr("确认清空日志文件？"));
		});
}

void SettingsPage::loadCurrentSettings()
{
	BENCHMARKING_FUNCTION();
	// 常规设置
	m_autoStartToggle->setChecked(m_configManager->getValue("app/autoStart").toBool());
	m_checkUpdatesToggle->setChecked(m_configManager->getValue("app/checkForUpdates").toBool());

	bool minimizeToTray = m_configManager->getValue("ui/minimizeToTray").toBool();
	if (minimizeToTray) {
		m_minimizeToTrayRadio->setChecked(true);
	}
	else {
		m_exitProgramRadio->setChecked(true);
	}

	// 语言设置
	QString language = m_configManager->getValue("ui/language").toString();
	m_languageCombo->setCurrentText(language);

	// 下载设置 - 修复路径问题
	QString defaultDownloadPath = m_configManager->getValue("download/defaultSavePath").toString();
	if (defaultDownloadPath.isEmpty()) {
		defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";
		QDir dir(defaultDownloadPath);
		if (!dir.exists()) {
			dir.mkpath(".");
		}
	}
	m_downloadPathInput->setText(defaultDownloadPath);

	QString videoQuality = m_configManager->getValue("download/defaultVideoQuality").toString();
	if (videoQuality == "最高质量") m_videoQualityHighest->setChecked(true);
	else if (videoQuality == "1080P") m_videoQuality1080P->setChecked(true);
	else if (videoQuality == "720P") m_videoQuality720P->setChecked(true);
	else if (videoQuality == "480P") m_videoQuality480P->setChecked(true);
	else if (videoQuality == "360P") m_videoQuality360P->setChecked(true);

	QString audioQuality = m_configManager->getValue("download/defaultAudioQuality").toString();
	if (audioQuality == "最高质量") m_audioQualityHighest->setChecked(true);
	else if (audioQuality == "320kbps") m_audioQuality320k->setChecked(true);
	else if (audioQuality == "256kbps") m_audioQuality256k->setChecked(true);
	else if (audioQuality == "192kbps") m_audioQuality192k->setChecked(true);
	else if (audioQuality == "128kbps") m_audioQuality128k->setChecked(true);

	QString downloadFormat = m_configManager->getValue("download/defaultFormat").toString();
	if (downloadFormat == "视频+音频(合并)") m_formatMerge->setChecked(true);
	else if (downloadFormat == "仅视频") m_formatVideoOnly->setChecked(true);
	else if (downloadFormat == "仅音频") m_formatAudioOnly->setChecked(true);
	else if (downloadFormat == "视频+音频(分离)") m_formatSeparate->setChecked(true);

	int concurrentDownloads = m_configManager->getValue("download/maxConcurrentDownloads").toInt();
	if (concurrentDownloads == 1) m_concurrent1->setChecked(true);
	else if (concurrentDownloads == 2) m_concurrent2->setChecked(true);
	else if (concurrentDownloads == 3) m_concurrent3->setChecked(true);
	else if (concurrentDownloads == 4) m_concurrent4->setChecked(true);
	else if (concurrentDownloads == 5) m_concurrent5->setChecked(true);

	// 网络设置
	QVariantMap proxyConfig = m_configManager->getValue("network/proxy").toMap();
	m_proxyEnabledToggle->setChecked(proxyConfig.value("enabled").toBool());

	QString proxyType = proxyConfig.value("type").toString();
	m_proxyTypeCombo->setCurrentText(proxyType);

	m_proxyHostInput->setText(proxyConfig.value("host").toString());
	m_proxyPortInput->setText(proxyConfig.value("port").toString());
	m_proxyUserInput->setText(proxyConfig.value("username").toString());
	m_proxyPassInput->setText(proxyConfig.value("password").toString());

	m_timeoutInput->setText(QString::number(m_configManager->getValue("network/timeout").toInt()));
	m_retryCountInput->setText(QString::number(m_configManager->getValue("network/retryCount").toInt()));
	m_userAgentInput->setText(m_configManager->getValue("network/userAgent").toString());

	// 高级设置 - 修复日志路径问题
	QString defaultLogPath = m_configManager->getValue("log/path").toString();
	if (defaultLogPath.isEmpty()) {
		// 如果配置中没有值，使用应用程序数据目录
		defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";

		// 确保日志目录存在
		QDir logDir(defaultLogPath);
		if (!logDir.exists()) {
			logDir.mkpath(".");
		}
	}
	m_logPathInput->setText(defaultLogPath);

	// 更新代理设置状态
	onProxySettingsChanged();
}

void SettingsPage::saveCurrentSettings()
{
	BENCHMARKING_FUNCTION();
	// 常规设置
	m_configManager->setValue("app/autoStart", m_autoStartToggle->isChecked());
	m_configManager->setValue("app/checkForUpdates", m_checkUpdatesToggle->isChecked());
	m_configManager->setValue("ui/minimizeToTray", m_minimizeToTrayRadio->isChecked());
	m_configManager->setValue("ui/language", m_languageCombo->currentText());

	// 下载设置
	m_configManager->setValue("download/defaultSavePath", m_downloadPathInput->text());

	QString videoQuality = "最高质量";
	if (m_videoQuality1080P->isChecked()) videoQuality = "1080P";
	else if (m_videoQuality720P->isChecked()) videoQuality = "720P";
	else if (m_videoQuality480P->isChecked()) videoQuality = "480P";
	else if (m_videoQuality360P->isChecked()) videoQuality = "360P";
	m_configManager->setValue("download/defaultVideoQuality", videoQuality);

	QString audioQuality = "最高质量";
	if (m_audioQuality320k->isChecked()) audioQuality = "320kbps";
	else if (m_audioQuality256k->isChecked()) audioQuality = "256kbps";
	else if (m_audioQuality192k->isChecked()) audioQuality = "192kbps";
	else if (m_audioQuality128k->isChecked()) audioQuality = "128kbps";
	m_configManager->setValue("download/defaultAudioQuality", audioQuality);

	QString downloadFormat = "视频+音频(合并)";
	if (m_formatVideoOnly->isChecked()) downloadFormat = "仅视频";
	else if (m_formatAudioOnly->isChecked()) downloadFormat = "仅音频";
	else if (m_formatSeparate->isChecked()) downloadFormat = "视频+音频(分离)";
	m_configManager->setValue("download/defaultFormat", downloadFormat);

	int concurrentDownloads = 3;
	if (m_concurrent1->isChecked()) concurrentDownloads = 1;
	else if (m_concurrent2->isChecked()) concurrentDownloads = 2;
	else if (m_concurrent3->isChecked()) concurrentDownloads = 3;
	else if (m_concurrent4->isChecked()) concurrentDownloads = 4;
	else if (m_concurrent5->isChecked()) concurrentDownloads = 5;
	m_configManager->setValue("download/maxConcurrentDownloads", concurrentDownloads);

	// 网络设置
	QVariantMap proxyConfig;
	proxyConfig["enabled"] = m_proxyEnabledToggle->isChecked();
	proxyConfig["type"] = m_proxyTypeCombo->currentText();
	proxyConfig["host"] = m_proxyHostInput->text();
	proxyConfig["port"] = m_proxyPortInput->text();
	proxyConfig["username"] = m_proxyUserInput->text();
	proxyConfig["password"] = m_proxyPassInput->text();
	m_configManager->setValue("network/proxy", proxyConfig);

	m_configManager->setValue("network/timeout", m_timeoutInput->text().toInt());
	m_configManager->setValue("network/retryCount", m_retryCountInput->text().toInt());
	m_configManager->setValue("network/userAgent", m_userAgentInput->text());

	// 高级设置
	m_configManager->setValue("log/path", m_logPathInput->text());

	// 保存配置
	m_configManager->save();
}

void SettingsPage::autoSaveSettings()
{
	saveCurrentSettings();
}


void SettingsPage::clearLog()
{
	//QString logPath = m_logPathInput->text();

	//// 如果日志路径为空，使用默认路径
	//if (logPath.isEmpty()) {
	//	logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	//	if (logPath.isEmpty()) {
	//		logPath = QCoreApplication::applicationDirPath() + "/logs";
	//	}
	//	else {
	//		logPath += "/logs";
	//	}
	//}

	//QDir logDir(logPath);

	//// 检查日志目录是否存在
	//if (!logDir.exists()) {
	//	//info
	//	return;
	//}

	//// 获取所有日志文件
	//QStringList logFilters;
	//logFilters << "*.log" << "*.txt" << "log_*" << "*.log.*";
	//QStringList logFiles = logDir.entryList(logFilters, QDir::Files | QDir::NoDotAndDotDot);

	//if (logFiles.isEmpty()) {
	//	//QMessageBox::information(
	//	//	this,
	//	//	"无需清空",
	//	//	"没有找到可清除的日志文件。"
	//	//);
	//	qDebug() << "没有找到可清除的日志文件。";
	//	return;
	//}

	//int deletedCount = 0;
	//int failedCount = 0;
	//qint64 totalFreedSpace = 0;

	//// 删除每个日志文件
	//for (const QString& fileName : logFiles) {
	//	QString filePath = logDir.absoluteFilePath(fileName);
	//	QFileInfo fileInfo(filePath);

	//	// 记录文件大小
	//	qint64 fileSize = fileInfo.size();

	//	if (QFile::remove(filePath)) {
	//		deletedCount++;
	//		totalFreedSpace += fileSize;
	//		LOG_DEBUG("Settings", "Deleted log file: %s", fileName.toUtf8().constData());
	//	}
	//	else {
	//		failedCount++;
	//		LOG_ERROR("Settings", "Failed to delete log file: %s", fileName.toUtf8().constData());
	//	}
	//}

	//// 显示结果
	//QString resultMessage;
	//if (deletedCount > 0) {
	//	QString sizeText;
	//	if (totalFreedSpace < 1024) {
	//		sizeText = QString("%1 字节").arg(totalFreedSpace);
	//	}
	//	else if (totalFreedSpace < 1024 * 1024) {
	//		sizeText = QString("%1 KB").arg(totalFreedSpace / 1024.0, 0, 'f', 2);
	//	}
	//	else {
	//		sizeText = QString("%1 MB").arg(totalFreedSpace / (1024.0 * 1024.0), 0, 'f', 2);
	//	}

	//	resultMessage = QString("成功清空 %1 个日志文件，释放 %2 空间。").arg(deletedCount).arg(sizeText);

	//	if (failedCount > 0) {
	//		resultMessage += QString("\n%1 个文件删除失败。").arg(failedCount);
	//	}
	//}
	//else {
	//	resultMessage = "没有成功删除任何日志文件。";
	//}

	////QMessageBox::information(this, "清空完成", resultMessage);

	//// 记录操作结果
	//if (deletedCount > 0) {
	//	LOG_INFO("Settings", "Cleared %d log files, freed %lld bytes", deletedCount, totalFreedSpace);
	//}
	//if (failedCount > 0) {
	//	LOG_WARN("Settings", "Failed to delete %d log files", failedCount);
	//}
}

// 获取默认设置
QVariantMap SettingsPage::getDefaultSettings() const
{
	BENCHMARKING_FUNCTION();
	// 直接从ConfigManager获取默认配置
	QJsonObject defaultJson = ConfigManager::getDefaultConfig();
	QVariantMap defaultSettings;

	// 转换为QVariantMap
	for (auto it = defaultJson.begin(); it != defaultJson.end(); ++it) {
		defaultSettings[it.key()] = it.value().toVariant();
	}

	// 动态设置路径（不能硬编码）
	QString defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";

	defaultSettings["download/defaultSavePath"] = defaultDownloadPath;

	QString defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";

	defaultSettings["log/path"] = defaultLogPath;

	return defaultSettings;
}

// 应用默认设置
void SettingsPage::applyDefaultSettings()
{
	// 应用所有默认设置
	QVariantMap defaultSettings = getDefaultSettings();

	for (auto it = defaultSettings.begin(); it != defaultSettings.end(); ++it) {
		m_configManager->setValue(it.key(), it.value());
	}

	// 保存配置
	m_configManager->save();

	// 记录日志
	LOG_INFO("All settings have been reset to default values");
}

// 重置常规设置
void SettingsPage::resetGeneralSettings()
{
	BENCHMARKING_FUNCTION();
	// 使用ConfigManager的静态方法获取默认配置
	QJsonObject defaultConfig = ConfigManager::getDefaultConfig();

	m_configManager->setValue("app/autoStart", defaultConfig["app/autoStart"].toVariant());
	m_configManager->setValue("app/checkForUpdates", defaultConfig["app/checkForUpdates"].toVariant());
	m_configManager->setValue("ui/minimizeToTray", defaultConfig["ui/minimizeToTray"].toVariant());
	m_configManager->setValue("ui/language", defaultConfig["ui/language"].toVariant());
	m_configManager->setValue("ui/theme", defaultConfig["ui/theme"].toVariant());

	LOG_INFO("General settings have been reset to defaults");
}

// 重置下载设置
void SettingsPage::resetDownloadSettings()
{
	BENCHMARKING_FUNCTION();
	// 设置默认下载路径
	QString defaultDownloadPath = QCoreApplication::applicationDirPath() + "/Downloads";

	// 使用ConfigManager的静态方法获取默认配置
	QJsonObject defaultConfig = ConfigManager::getDefaultConfig();

	m_configManager->setValue("download/defaultSavePath", defaultDownloadPath);
	m_configManager->setValue("download/defaultVideoQuality", defaultConfig["download/defaultVideoQuality"].toVariant());
	m_configManager->setValue("download/defaultAudioQuality", defaultConfig["download/defaultAudioQuality"].toVariant());
	m_configManager->setValue("download/defaultFormat", defaultConfig["download/defaultFormat"].toVariant());
	m_configManager->setValue("download/maxConcurrentDownloads", defaultConfig["download/maxConcurrentDownloads"].toVariant());
	m_configManager->setValue("download/autoMerge", defaultConfig["download/autoMerge"].toVariant());
	m_configManager->setValue("download/autoDeleteTempFiles", defaultConfig["download/autoDeleteTempFiles"].toVariant());

	LOG_INFO("Download settings have been reset to defaults");
}

// 重置网络设置
void SettingsPage::resetNetworkSettings()
{
	BENCHMARKING_FUNCTION();
	// 使用ConfigManager的静态方法获取默认配置
	QJsonObject defaultConfig = ConfigManager::getDefaultConfig();

	m_configManager->setValue("network/timeout", defaultConfig["network/timeout"].toVariant());
	m_configManager->setValue("network/retryCount", defaultConfig["network/retryCount"].toVariant());
	m_configManager->setValue("network/userAgent", defaultConfig["network/userAgent"].toVariant());

	// 重置代理设置
	QVariantMap proxyConfig;
	proxyConfig["enabled"] = defaultConfig["network/proxy/enabled"].toVariant();
	proxyConfig["type"] = defaultConfig["network/proxy/type"].toVariant();
	proxyConfig["host"] = defaultConfig["network/proxy/host"].toVariant();
	proxyConfig["port"] = defaultConfig["network/proxy/port"].toVariant();
	proxyConfig["username"] = defaultConfig["network/proxy/username"].toVariant();
	proxyConfig["password"] = defaultConfig["network/proxy/password"].toVariant();
	m_configManager->setValue("network/proxy", proxyConfig);

	LOG_INFO("Network settings have been reset to defaults");
}

// 重置高级设置
void SettingsPage::resetAdvancedSettings()
{
	BENCHMARKING_FUNCTION();
	// 设置默认日志路径
	QString defaultLogPath = QCoreApplication::applicationDirPath() + "/logs";

	// 使用ConfigManager的静态方法获取默认配置
	QJsonObject defaultConfig = ConfigManager::getDefaultConfig();

	m_configManager->setValue("log/path", defaultLogPath);
	m_configManager->setValue("log/level", defaultConfig["log/level"].toVariant());
	m_configManager->setValue("log/maxSize", defaultConfig["log/maxSize"].toVariant());
	m_configManager->setValue("log/maxFiles", defaultConfig["log/maxFiles"].toVariant());

	LOG_INFO("Advanced settings have been reset to defaults");
}

// 完整的重置设置实现
void SettingsPage::resetSettings()
{
	BENCHMARKING_FUNCTION();
	applyDefaultSettings();

	// 重新加载界面显示新设置
	loadCurrentSettings();

	LOG_INFO("All settings have been reset to default values by user");
}

void SettingsPage::onDownloadPathBrowse()
{
	QString path = QFileDialog::getExistingDirectory(this, tr("选择下载目录"),
		m_downloadPathInput->text());
	if (!path.isEmpty()) {
		m_downloadPathInput->setText(path);
		autoSaveSettings(); // 自动保存
	}
}

void SettingsPage::onExitBehaviorChanged()
{
	// 退出行为改变的处理
	LOG_INFO(QString(tr("退出行为更改为: %1"))
		.arg(m_minimizeToTrayRadio->isChecked() ? tr("最小化到系统托盘") : tr("退出程序")));
}

void SettingsPage::onProxySettingsChanged()
{
	BENCHMARKING_FUNCTION();
	bool enabled = m_proxyEnabledToggle->isChecked();
	m_proxyTypeCombo->setEnabled(enabled);
	m_proxyHostInput->setEnabled(enabled);
	m_proxyPortInput->setEnabled(enabled);
	m_proxyUserInput->setEnabled(enabled);
	m_proxyPassInput->setEnabled(enabled);
}