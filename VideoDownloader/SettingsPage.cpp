#include "SettingsPage.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFormLayout>
#include <QScrollArea>
#include "ModManager.h"
#include "NetworkManager.h"

SettingsPage::SettingsPage(QSharedPointer<ApplicationController> appController, QWidget* parent)
	: QWidget(parent)
	, m_appController(appController)
{
	// 从 ApplicationController 获取 ConfigManager
	m_configManager = m_appController->getConfigManager();

	setObjectName("SettingsPage");
	setupUI();
	setupConnections();
	loadCurrentSettings();
	applyTheme();
}

SettingsPage::~SettingsPage()
{
}

void SettingsPage::setupUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(20);
	m_mainLayout->setContentsMargins(20, 20, 20, 20);

	// 创建选项卡
	m_tabWidget = new QTabWidget(this);
	m_tabWidget->setObjectName("SettingsTabWidget");

	// 设置各个选项卡
	setupGeneralSettings();
	setupDownloadSettings();
	setupNetworkSettings();
	setupUISettings();
	setupModSettings();
	setupAdvancedSettings();

	m_mainLayout->addWidget(m_tabWidget);

	// 底部按钮
	m_buttonLayout = new QHBoxLayout();
	m_buttonLayout->setSpacing(15);
	m_buttonLayout->setAlignment(Qt::AlignRight);

	m_saveButton = new AntButton("保存设置", 12, this);
	m_saveButton->setFixedSize(120, 40);

	m_resetButton = new AntButton("重置设置", 12, this);
	m_resetButton->setFixedSize(120, 40);

	m_cancelButton = new AntButton("取消", 12, this);
	m_cancelButton->setFixedSize(120, 40);

	m_buttonLayout->addStretch();
	m_buttonLayout->addWidget(m_saveButton);
	m_buttonLayout->addWidget(m_resetButton);
	m_buttonLayout->addWidget(m_cancelButton);

	m_mainLayout->addLayout(m_buttonLayout);
}

void SettingsPage::setupGeneralSettings()
{
	m_generalTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_generalTab);

	QGroupBox* generalGroup = new QGroupBox("常规设置", m_generalTab);
	QFormLayout* formLayout = new QFormLayout(generalGroup);

	m_appNameInput = new QLineEdit(this);
	m_appNameInput->setPlaceholderText("视频下载器");
	formLayout->addRow("应用名称:", m_appNameInput);

	m_organizationInput = new QLineEdit(this);
	m_organizationInput->setPlaceholderText("VideoDownloader");
	formLayout->addRow("组织名称:", m_organizationInput);

	m_autoStartToggle = new AntToggleButton(QSize(57, 26), this);
	m_autoStartToggle->setShowText(true);
	formLayout->addRow("开机自启动:", m_autoStartToggle);

	m_checkUpdatesToggle = new AntToggleButton(QSize(57, 26), this);
	m_checkUpdatesToggle->setShowText(true);
	formLayout->addRow("自动检查更新:", m_checkUpdatesToggle);

	m_minimizeToTrayToggle = new AntToggleButton(QSize(57, 26), this);
	m_minimizeToTrayToggle->setShowText(true);
	formLayout->addRow("最小化到系统托盘:", m_minimizeToTrayToggle);

	layout->addWidget(generalGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_generalTab, "常规");
}

void SettingsPage::setupDownloadSettings()
{
	m_downloadTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_downloadTab);

	QGroupBox* downloadGroup = new QGroupBox("下载设置", m_downloadTab);
	QFormLayout* formLayout = new QFormLayout(downloadGroup);

	// 下载路径
	QHBoxLayout* pathLayout = new QHBoxLayout();
	m_downloadPathInput = new QLineEdit(this);
	m_browsePathButton = new AntButton("浏览", 10, this);
	m_browsePathButton->setFixedSize(60, 35);
	pathLayout->addWidget(m_downloadPathInput);
	pathLayout->addWidget(m_browsePathButton);
	formLayout->addRow("默认下载路径:", pathLayout);

	// 视频质量
	m_videoQualityCombo = new QComboBox(this);
	m_videoQualityCombo->addItems(QStringList() << "最高质量" << "1080P" << "720P" << "480P" << "360P");
	formLayout->addRow("默认视频质量:", m_videoQualityCombo);

	// 音频质量
	m_audioQualityCombo = new QComboBox(this);
	m_audioQualityCombo->addItems(QStringList() << "最高质量" << "320kbps" << "256kbps" << "192kbps" << "128kbps");
	formLayout->addRow("默认音频质量:", m_audioQualityCombo);

	// 下载格式
	m_downloadFormatCombo = new QComboBox(this);
	m_downloadFormatCombo->addItems(QStringList() << "视频+音频(合并)" << "仅视频" << "仅音频" << "视频+音频(分离)");
	formLayout->addRow("默认下载格式:", m_downloadFormatCombo);

	// 同时下载数量
	QHBoxLayout* concurrentLayout = new QHBoxLayout();
	m_maxConcurrentSlider = new AntSlider(1, 10, 3, this);
	m_maxConcurrentLabel = new QLabel("3", this);
	concurrentLayout->addWidget(m_maxConcurrentSlider);
	concurrentLayout->addWidget(m_maxConcurrentLabel);
	formLayout->addRow("同时下载数量:", concurrentLayout);

	m_autoMergeToggle = new AntToggleButton(QSize(57, 26), this);
	m_autoMergeToggle->setShowText(true);
	formLayout->addRow("自动合并音视频:", m_autoMergeToggle);

	m_autoDeleteTempToggle = new AntToggleButton(QSize(57, 26), this);
	m_autoDeleteTempToggle->setShowText(true);
	formLayout->addRow("自动删除临时文件:", m_autoDeleteTempToggle);

	layout->addWidget(downloadGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_downloadTab, "下载");
}

void SettingsPage::setupNetworkSettings()
{
	m_networkTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_networkTab);

	QGroupBox* proxyGroup = new QGroupBox("代理设置", m_networkTab);
	QFormLayout* proxyLayout = new QFormLayout(proxyGroup);

	m_proxyEnabledToggle = new AntToggleButton(QSize(57, 26), this);
	m_proxyEnabledToggle->setShowText(true);
	proxyLayout->addRow("启用代理:", m_proxyEnabledToggle);

	m_proxyTypeCombo = new QComboBox(this);
	m_proxyTypeCombo->addItems(QStringList() << "HTTP" << "SOCKS5" << "HTTPS");
	proxyLayout->addRow("代理类型:", m_proxyTypeCombo);

	m_proxyHostInput = new QLineEdit(this);
	m_proxyHostInput->setPlaceholderText("proxy.example.com");
	proxyLayout->addRow("代理主机:", m_proxyHostInput);

	m_proxyPortInput = new QLineEdit(this);
	m_proxyPortInput->setPlaceholderText("8080");
	proxyLayout->addRow("代理端口:", m_proxyPortInput);

	m_proxyUserInput = new QLineEdit(this);
	m_proxyUserInput->setPlaceholderText("用户名");
	proxyLayout->addRow("代理用户名:", m_proxyUserInput);

	m_proxyPassInput = new QLineEdit(this);
	m_proxyPassInput->setPlaceholderText("密码");
	m_proxyPassInput->setEchoMode(QLineEdit::Password);
	proxyLayout->addRow("代理密码:", m_proxyPassInput);

	QGroupBox* connectionGroup = new QGroupBox("连接设置", m_networkTab);
	QFormLayout* connectionLayout = new QFormLayout(connectionGroup);

	m_timeoutInput = new QSpinBox(this);
	m_timeoutInput->setRange(1000, 60000);
	m_timeoutInput->setSingleStep(1000);
	m_timeoutInput->setSuffix(" ms");
	connectionLayout->addRow("超时时间:", m_timeoutInput);

	m_retryCountInput = new QSpinBox(this);
	m_retryCountInput->setRange(0, 10);
	connectionLayout->addRow("重试次数:", m_retryCountInput);

	m_userAgentInput = new QLineEdit(this);
	m_userAgentInput->setPlaceholderText("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
	connectionLayout->addRow("用户代理:", m_userAgentInput);

	layout->addWidget(proxyGroup);
	layout->addWidget(connectionGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_networkTab, "网络");
}

void SettingsPage::setupUISettings()
{
	m_uiTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_uiTab);

	QGroupBox* appearanceGroup = new QGroupBox("外观设置", m_uiTab);
	QFormLayout* formLayout = new QFormLayout(appearanceGroup);

	m_themeCombo = new QComboBox(this);
	m_themeCombo->addItems(QStringList() << "浅色主题" << "深色主题" << "跟随系统");
	formLayout->addRow("界面主题:", m_themeCombo);

	m_languageCombo = new QComboBox(this);
	m_languageCombo->addItems(QStringList() << "简体中文" << "English" << "日本語");
	formLayout->addRow("界面语言:", m_languageCombo);

	m_startupPageCombo = new QComboBox(this);
	m_startupPageCombo->addItems(QStringList() << "主页" << "下载页面" << "上次关闭时的页面");
	formLayout->addRow("启动页面:", m_startupPageCombo);

	m_fontSizeCombo = new QComboBox(this);
	m_fontSizeCombo->addItems(QStringList() << "小" << "中" << "大" << "自定义");
	formLayout->addRow("字体大小:", m_fontSizeCombo);

	QGroupBox* behaviorGroup = new QGroupBox("行为设置", m_uiTab);
	QFormLayout* behaviorLayout = new QFormLayout(behaviorGroup);

	m_showTrayIconToggle = new AntToggleButton(QSize(57, 26), this);
	m_showTrayIconToggle->setShowText(true);
	behaviorLayout->addRow("显示系统托盘图标:", m_showTrayIconToggle);

	m_closeToTrayToggle = new AntToggleButton(QSize(57, 26), this);
	m_closeToTrayToggle->setShowText(true);
	behaviorLayout->addRow("关闭时最小化到托盘:", m_closeToTrayToggle);

	layout->addWidget(appearanceGroup);
	layout->addWidget(behaviorGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_uiTab, "界面");
}

void SettingsPage::setupModSettings()
{
	m_modTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_modTab);

	QGroupBox* modGroup = new QGroupBox("Mod 管理", m_modTab);
	QVBoxLayout* modLayout = new QVBoxLayout(modGroup);

	// Mod 列表
	QLabel* modListLabel = new QLabel("已加载的 Mod:", this);
	modLayout->addWidget(modListLabel);

	m_modListLayout = new QVBoxLayout();
	modLayout->addLayout(m_modListLayout);

	// Mod 设置
	m_autoUpdateModsToggle = new AntToggleButton(QSize(57, 26), this);
	m_autoUpdateModsToggle->setShowText(true);
	modLayout->addWidget(m_autoUpdateModsToggle);

	QHBoxLayout* modButtonsLayout = new QHBoxLayout();
	m_refreshModsButton = new AntButton("刷新 Mod 列表", 10, this);
	m_openModsFolderButton = new AntButton("打开 Mod 文件夹", 10, this);

	modButtonsLayout->addWidget(m_refreshModsButton);
	modButtonsLayout->addWidget(m_openModsFolderButton);
	modLayout->addLayout(modButtonsLayout);

	layout->addWidget(modGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_modTab, "Mod");
}

void SettingsPage::setupAdvancedSettings()
{
	m_advancedTab = new QWidget(this);
	QVBoxLayout* layout = new QVBoxLayout(m_advancedTab);

	QGroupBox* logGroup = new QGroupBox("日志设置", m_advancedTab);
	QFormLayout* logLayout = new QFormLayout(logGroup);

	m_logLevelCombo = new QComboBox(this);
	m_logLevelCombo->addItems(QStringList() << "跟踪" << "调试" << "信息" << "警告" << "错误" << "严重");
	logLayout->addRow("日志级别:", m_logLevelCombo);

	QHBoxLayout* logPathLayout = new QHBoxLayout();
	m_logPathInput = new QLineEdit(this);
	m_browseLogPathButton = new AntButton("浏览", 10, this);
	m_browseLogPathButton->setFixedSize(60, 35);
	logPathLayout->addWidget(m_logPathInput);
	logPathLayout->addWidget(m_browseLogPathButton);
	logLayout->addRow("日志路径:", logPathLayout);

	QHBoxLayout* logButtonsLayout = new QHBoxLayout();
	m_viewLogsButton = new AntButton("查看日志", 10, this);
	m_clearLogsButton = new AntButton("清空日志", 10, this);
	logButtonsLayout->addWidget(m_viewLogsButton);
	logButtonsLayout->addWidget(m_clearLogsButton);
	logLayout->addRow("日志操作:", logButtonsLayout);

	QGroupBox* debugGroup = new QGroupBox("调试设置", m_advancedTab);
	QFormLayout* debugLayout = new QFormLayout(debugGroup);

	m_debugModeToggle = new AntToggleButton(QSize(57, 26), this);
	m_debugModeToggle->setShowText(true);
	debugLayout->addRow("调试模式:", m_debugModeToggle);

	m_resetAllSettingsButton = new AntButton("重置所有设置", 10, this);
	debugLayout->addRow("危险操作:", m_resetAllSettingsButton);

	layout->addWidget(logGroup);
	layout->addWidget(debugGroup);
	layout->addStretch();

	m_tabWidget->addTab(m_advancedTab, "高级");
}

void SettingsPage::setupConnections()
{
	connect(m_saveButton, &AntButton::clicked, this, &SettingsPage::onSaveSettings);
	connect(m_resetButton, &AntButton::clicked, this, &SettingsPage::onResetSettings);
	connect(m_cancelButton, &AntButton::clicked, this, [this]() {
		if (parentWidget()) {
			parentWidget()->close();
		}
		});

	connect(m_browsePathButton, &AntButton::clicked, this, &SettingsPage::onDownloadPathBrowse);
	connect(m_maxConcurrentSlider, &AntSlider::valueChanged, this, &SettingsPage::onMaxConcurrentDownloadsChanged);
	connect(m_proxyEnabledToggle, &AntToggleButton::toggled, this, &SettingsPage::onProxySettingsChanged);
	connect(m_logLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onLogLevelChanged);
	connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);

	connect(m_refreshModsButton, &AntButton::clicked, this, [this]() {
		if (m_appController->getModManager()) {
			m_appController->getModManager()->discoverMods();
			loadCurrentSettings(); // 重新加载Mod设置
		}
		});

	connect(m_openModsFolderButton, &AntButton::clicked, this, [this]() {
		QString modsDir = QCoreApplication::applicationDirPath() + "/mods";
		QDir dir(modsDir);
		if (!dir.exists()) {
			dir.mkpath(".");
		}
		QDesktopServices::openUrl(QUrl::fromLocalFile(modsDir));
		});

	connect(m_viewLogsButton, &AntButton::clicked, this, [this]() {
		QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
		QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
		});

	connect(m_clearLogsButton, &AntButton::clicked, this, [this]() {
		QMessageBox::StandardButton reply = QMessageBox::question(this, "确认清空",
			"确定要清空所有日志文件吗？此操作不可撤销。",
			QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::Yes) {
			QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
			QDir dir(logDir);
			dir.removeRecursively();
			dir.mkpath(".");
			QMessageBox::information(this, "完成", "日志文件已清空");
		}
		});

	connect(m_resetAllSettingsButton, &AntButton::clicked, this, [this]() {
		QMessageBox::StandardButton reply = QMessageBox::warning(this, "确认重置",
			"确定要重置所有设置吗？此操作将恢复所有设置为默认值，且不可撤销。",
			QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::Yes) {
			m_configManager->setValue("app/firstRun", true);
			m_configManager->save();
			QMessageBox::information(this, "完成", "设置已重置，请重启应用");
		}
		});
}

void SettingsPage::loadCurrentSettings()
{
	// 常规设置
	m_appNameInput->setText(m_configManager->getValue("app/name", "视频下载器").toString());
	m_organizationInput->setText(m_configManager->getValue("app/organization", "VideoDownloader").toString());
	m_autoStartToggle->setChecked(m_configManager->getValue("app/autoStart", false).toBool());
	m_checkUpdatesToggle->setChecked(m_configManager->getValue("app/checkForUpdates", true).toBool());
	m_minimizeToTrayToggle->setChecked(m_configManager->getValue("ui/minimizeToTray", true).toBool());

	// 下载设置
	m_downloadPathInput->setText(m_configManager->getValue("download/defaultSavePath",
		QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString());

	QString videoQuality = m_configManager->getValue("download/defaultVideoQuality", "最高质量").toString();
	int videoIndex = m_videoQualityCombo->findText(videoQuality);
	if (videoIndex >= 0) m_videoQualityCombo->setCurrentIndex(videoIndex);

	QString audioQuality = m_configManager->getValue("download/defaultAudioQuality", "最高质量").toString();
	int audioIndex = m_audioQualityCombo->findText(audioQuality);
	if (audioIndex >= 0) m_audioQualityCombo->setCurrentIndex(audioIndex);

	QString downloadFormat = m_configManager->getValue("download/defaultFormat", "视频+音频(合并)").toString();
	int formatIndex = m_downloadFormatCombo->findText(downloadFormat);
	if (formatIndex >= 0) m_downloadFormatCombo->setCurrentIndex(formatIndex);

	m_maxConcurrentSlider->setValue(m_configManager->getValue("download/maxConcurrentDownloads", 3).toInt());
	m_maxConcurrentLabel->setText(QString::number(m_maxConcurrentSlider->value()));
	m_autoMergeToggle->setChecked(m_configManager->getValue("download/autoMerge", true).toBool());
	m_autoDeleteTempToggle->setChecked(m_configManager->getValue("download/autoDeleteTempFiles", true).toBool());

	// 网络设置
	QVariantMap proxyConfig = m_configManager->getValue("network/proxy").toMap();
	m_proxyEnabledToggle->setChecked(proxyConfig.value("enabled", false).toBool());

	QString proxyType = proxyConfig.value("type", "http").toString().toUpper();
	int typeIndex = m_proxyTypeCombo->findText(proxyType);
	if (typeIndex >= 0) m_proxyTypeCombo->setCurrentIndex(typeIndex);

	m_proxyHostInput->setText(proxyConfig.value("host").toString());
	m_proxyPortInput->setText(proxyConfig.value("port").toString());
	m_proxyUserInput->setText(proxyConfig.value("username").toString());
	m_proxyPassInput->setText(proxyConfig.value("password").toString());

	m_timeoutInput->setValue(m_configManager->getValue("network/timeout", 30000).toInt());
	m_retryCountInput->setValue(m_configManager->getValue("network/retryCount", 3).toInt());
	m_userAgentInput->setText(m_configManager->getValue("network/userAgent",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36").toString());

	// 界面设置
	QString theme = m_configManager->getValue("ui/theme", "浅色主题").toString();
	int themeIndex = m_themeCombo->findText(theme);
	if (themeIndex >= 0) m_themeCombo->setCurrentIndex(themeIndex);

	QString language = m_configManager->getValue("ui/language", "简体中文").toString();
	int langIndex = m_languageCombo->findText(language);
	if (langIndex >= 0) m_languageCombo->setCurrentIndex(langIndex);

	QString startupPage = m_configManager->getValue("ui/startupPage", "主页").toString();
	int startupIndex = m_startupPageCombo->findText(startupPage);
	if (startupIndex >= 0) m_startupPageCombo->setCurrentIndex(startupIndex);

	m_showTrayIconToggle->setChecked(m_configManager->getValue("ui/showTrayIcon", true).toBool());
	m_closeToTrayToggle->setChecked(m_configManager->getValue("ui/closeToTray", false).toBool());

	QString fontSize = m_configManager->getValue("ui/fontSize", "中").toString();
	int fontSizeIndex = m_fontSizeCombo->findText(fontSize);
	if (fontSizeIndex >= 0) m_fontSizeCombo->setCurrentIndex(fontSizeIndex);

	// Mod设置
	m_autoUpdateModsToggle->setChecked(m_configManager->getValue("mods/autoUpdateMods", false).toBool());

	// 加载Mod列表
	QLayoutItem* child;
	while ((child = m_modListLayout->takeAt(0)) != nullptr) {
		if (child->widget()) {
			child->widget()->deleteLater();
		}
		delete child;
	}

	if (m_appController->getModManager()) {
		auto modManager = m_appController->getModManager();
		auto loadedMods = modManager->getLoadedMods();

		for (const QString& modId : loadedMods) {
			auto modInfo = modManager->getModInfo(modId);
			QHBoxLayout* modItemLayout = new QHBoxLayout();

			AntToggleButton* modToggle = new AntToggleButton(QSize(57, 26), this);
			modToggle->setShowText(true);
			modToggle->setChecked(modManager->isModEnabled(modId));
			modToggle->setProperty("modId", modId);

			QLabel* modNameLabel = new QLabel(modInfo.metadata.value("name").toString() +
				" v" + modInfo.metadata.value("version").toString(), this);

			modItemLayout->addWidget(modToggle);
			modItemLayout->addWidget(modNameLabel);
			modItemLayout->addStretch();

			connect(modToggle, &AntToggleButton::toggled, this, &SettingsPage::onModEnabledToggled);

			m_modListLayout->addLayout(modItemLayout);
		}
	}

	// 高级设置
	int logLevel = m_configManager->getValue("log/level", LogSystem::Info).toInt();
	m_logLevelCombo->setCurrentIndex(logLevel);

	m_logPathInput->setText(m_configManager->getValue("log/path",
		QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs").toString());

	m_debugModeToggle->setChecked(m_configManager->getValue("debug/enabled", false).toBool());
}

void SettingsPage::saveCurrentSettings()
{
	// 常规设置
	m_configManager->setValue("app/name", m_appNameInput->text());
	m_configManager->setValue("app/organization", m_organizationInput->text());
	m_configManager->setValue("app/autoStart", m_autoStartToggle->isChecked());
	m_configManager->setValue("app/checkForUpdates", m_checkUpdatesToggle->isChecked());
	m_configManager->setValue("ui/minimizeToTray", m_minimizeToTrayToggle->isChecked());

	// 下载设置
	m_configManager->setValue("download/defaultSavePath", m_downloadPathInput->text());
	m_configManager->setValue("download/defaultVideoQuality", m_videoQualityCombo->currentText());
	m_configManager->setValue("download/defaultAudioQuality", m_audioQualityCombo->currentText());
	m_configManager->setValue("download/defaultFormat", m_downloadFormatCombo->currentText());
	m_configManager->setValue("download/maxConcurrentDownloads", m_maxConcurrentSlider->value());
	m_configManager->setValue("download/autoMerge", m_autoMergeToggle->isChecked());
	m_configManager->setValue("download/autoDeleteTempFiles", m_autoDeleteTempToggle->isChecked());

	// 网络设置
	QVariantMap proxyConfig;
	proxyConfig["enabled"] = m_proxyEnabledToggle->isChecked();
	proxyConfig["type"] = m_proxyTypeCombo->currentText().toLower();
	proxyConfig["host"] = m_proxyHostInput->text();
	proxyConfig["port"] = m_proxyPortInput->text().toInt();
	proxyConfig["username"] = m_proxyUserInput->text();
	proxyConfig["password"] = m_proxyPassInput->text();
	m_configManager->setValue("network/proxy", proxyConfig);

	m_configManager->setValue("network/timeout", m_timeoutInput->value());
	m_configManager->setValue("network/retryCount", m_retryCountInput->value());
	m_configManager->setValue("network/userAgent", m_userAgentInput->text());

	// 界面设置
	m_configManager->setValue("ui/theme", m_themeCombo->currentText());
	m_configManager->setValue("ui/language", m_languageCombo->currentText());
	m_configManager->setValue("ui/startupPage", m_startupPageCombo->currentText());
	m_configManager->setValue("ui/showTrayIcon", m_showTrayIconToggle->isChecked());
	m_configManager->setValue("ui/closeToTray", m_closeToTrayToggle->isChecked());
	m_configManager->setValue("ui/fontSize", m_fontSizeCombo->currentText());

	// Mod设置
	m_configManager->setValue("mods/autoUpdateMods", m_autoUpdateModsToggle->isChecked());

	// 高级设置
	m_configManager->setValue("log/level", m_logLevelCombo->currentIndex());
	m_configManager->setValue("log/path", m_logPathInput->text());
	m_configManager->setValue("debug/enabled", m_debugModeToggle->isChecked());

	// 保存配置
	m_configManager->save();
}

void SettingsPage::applyTheme()
{
	QString styleSheet = QString(
		"QWidget#SettingsPage {"
		"    background-color: %1;"
		"}"
		"QTabWidget::pane {"
		"    border: 1px solid %2;"
		"    background-color: %1;"
		"}"
		"QTabBar::tab {"
		"    background-color: %3;"
		"    color: %4;"
		"    padding: 8px 16px;"
		"    margin-right: 2px;"
		"}"
		"QTabBar::tab:selected {"
		"    background-color: %5;"
		"    color: %6;"
		"}"
		"QGroupBox {"
		"    font-weight: bold;"
		"    color: %4;"
		"    border: 1px solid %2;"
		"    margin-top: 10px;"
		"    padding-top: 10px;"
		"}"
		"QGroupBox::title {"
		"    subcontrol-origin: margin;"
		"    left: 10px;"
		"    padding: 0 5px 0 5px;"
		"}"
		"QLabel {"
		"    color: %4;"
		"}"
		"QLineEdit, QComboBox, QSpinBox {"
		"    background-color: %3;"
		"    color: %4;"
		"    border: 1px solid %2;"
		"    padding: 5px;"
		"    border-radius: 3px;"
		"}"
	).arg(
		DesignSystem::instance()->backgroundColor().name(),
		DesignSystem::instance()->borderColor().name(),
		DesignSystem::instance()->widgetBgColor().name(),
		DesignSystem::instance()->primaryTextColor().name(),
		DesignSystem::instance()->primaryColor().name(),
		DesignSystem::instance()->textColor().name()
	);

	setStyleSheet(styleSheet);
}

void SettingsPage::onSaveSettings()
{
	saveCurrentSettings();

	// 应用网络设置 - 由于ApplicationController没有getNetworkManager，这里暂时注释
	/*
	if (m_appController->getNetworkManager()) {
		NetworkProxy proxy;
		proxy.enabled = m_proxyEnabledToggle->isChecked();
		proxy.type = m_proxyTypeCombo->currentText().toLower();
		proxy.host = m_proxyHostInput->text();
		proxy.port = m_proxyPortInput->text().toInt();
		proxy.username = m_proxyUserInput->text();
		proxy.password = m_proxyPassInput->text();

		m_appController->getNetworkManager()->setProxy(proxy);
		m_appController->getNetworkManager()->setTimeout(m_timeoutInput->text().toInt());
		m_appController->getNetworkManager()->setRetryCount(m_retryCountInput->text().toInt());
		m_appController->getNetworkManager()->setUserAgent(m_userAgentInput->text());
	}
	*/

	// 应用日志设置
	LogSystem::instance().setGlobalLevel(static_cast<LogSystem::LogLevel>(m_logLevelCombo->currentIndex()));

	QMessageBox::information(this, "保存成功", "设置已保存并应用");
}

void SettingsPage::onResetSettings()
{
	QMessageBox::StandardButton reply = QMessageBox::question(this, "确认重置",
		"确定要重置当前选项卡的设置吗？",
		QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::Yes) {
		loadCurrentSettings();
	}
}

void SettingsPage::onThemeChanged()
{
	QString theme = m_themeCombo->currentText();
	if (theme == "浅色主题") {
		DesignSystem::instance()->setThemeMode(DesignSystem::Light);
	}
	else if (theme == "深色主题") {
		DesignSystem::instance()->setThemeMode(DesignSystem::Dark);
	}
	applyTheme();
	emit DesignSystem::instance()->themeChanged();
}

void SettingsPage::onDownloadPathBrowse()
{
	QString path = QFileDialog::getExistingDirectory(this, "选择下载目录",
		m_downloadPathInput->text());
	if (!path.isEmpty()) {
		m_downloadPathInput->setText(path);
	}
}

void SettingsPage::onProxySettingsChanged()
{
	bool enabled = m_proxyEnabledToggle->isChecked();
	m_proxyTypeCombo->setEnabled(enabled);
	m_proxyHostInput->setEnabled(enabled);
	m_proxyPortInput->setEnabled(enabled);
	m_proxyUserInput->setEnabled(enabled);
	m_proxyPassInput->setEnabled(enabled);
}

void SettingsPage::onLogLevelChanged(int index)
{
	QString levelText;
	switch (index) {
	case 0: levelText = "跟踪"; break;
	case 1: levelText = "调试"; break;
	case 2: levelText = "信息"; break;
	case 3: levelText = "警告"; break;
	case 4: levelText = "错误"; break;
	case 5: levelText = "严重"; break;
	default: levelText = "信息";
	}

	LOG_INFO("Settings", QString("日志级别更改为: %1").arg(levelText));
}

void SettingsPage::onMaxConcurrentDownloadsChanged(int value)
{
	m_maxConcurrentLabel->setText(QString::number(value));
}

void SettingsPage::onModEnabledToggled(bool enabled)
{
	AntToggleButton* toggle = qobject_cast<AntToggleButton*>(sender());
	if (toggle && m_appController->getModManager()) {
		QString modId = toggle->property("modId").toString();
		if (enabled) {
			m_appController->getModManager()->enableMod(modId);
		}
		else {
			m_appController->getModManager()->disableMod(modId);
		}

		LOG_INFO("Settings", QString("Mod %1 %2").arg(modId).arg(enabled ? "已启用" : "已禁用"));
	}
}