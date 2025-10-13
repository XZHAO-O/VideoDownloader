#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QLineEdit>
#include <QDesktopServices>
#include <QApplication>
#include "ApplicationController.h"
#include "ConfigManager.h"
#include "DesignSystem.h"
#include "AntButton.h"
#include "AntToggleButton.h"
#include "AntRadioButton.h"
#include "AntComboBox.h"
#include "MaterialTabWidget.h"
#include "LogSystem.h"

class SettingsPage : public QWidget
{
	Q_OBJECT

public:
	SettingsPage(QSharedPointer<ApplicationController> appController, QWidget* parent = nullptr);
	~SettingsPage();

	void resetSettings();
	void clearLog();

signals:
	void showResetDialog(const QString& title, const QString& message);
	void showLogClearDialog(const QString& title, const QString& message);

private slots:
	void onDownloadPathBrowse();
	void onExitBehaviorChanged();
	void onProxySettingsChanged();

private:
	void setupUI();
	void setupConnections();
	void loadCurrentSettings();
	void saveCurrentSettings();
	void autoSaveSettings();

	// 设置类别
	void setupGeneralSettings();
	void setupDownloadSettings();
	void setupNetworkSettings();
	void setupAdvancedSettings();

	// 重置功能相关
	QVariantMap getDefaultSettings() const;
	void applyDefaultSettings();
	void resetGeneralSettings();
	void resetDownloadSettings();
	void resetNetworkSettings();
	void resetAdvancedSettings();

	// 默认配置常量
	static const QMap<QString, QVariant> DEFAULT_SETTINGS;

	QSharedPointer<ApplicationController> m_appController;
	QSharedPointer<ConfigManager> m_configManager;

	// 主布局和选项卡
	QVBoxLayout* m_mainLayout;
	MaterialTabWidget* m_tabWidget;

	// 常规设置
	QWidget* m_generalTab;
	QVBoxLayout* m_generalLayout;
	AntToggleButton* m_autoStartToggle;
	AntToggleButton* m_checkUpdatesToggle;
	QHBoxLayout* m_exitBehaviorLayout;
	QLabel* m_exitBehaviorLabel;
	AntRadioButton* m_exitProgramRadio;
	AntRadioButton* m_minimizeToTrayRadio;
	QHBoxLayout* m_languageLayout;
	QLabel* m_languageLabel;
	AntComboBox* m_languageCombo;
	AntButton* m_resetButton;

	// 下载设置
	QWidget* m_downloadTab;
	QVBoxLayout* m_downloadLayout;

	// 下载路径
	QHBoxLayout* m_downloadPathLayout;
	QLabel* m_downloadPathLabel;
	QLineEdit* m_downloadPathInput;
	AntButton* m_browsePathButton;

	// 视频质量
	QHBoxLayout* m_videoQualityLayout;
	QLabel* m_videoQualityLabel;
	AntRadioButton* m_videoQualityHighest;
	AntRadioButton* m_videoQuality1080P;
	AntRadioButton* m_videoQuality720P;
	AntRadioButton* m_videoQuality480P;
	AntRadioButton* m_videoQuality360P;

	// 音频质量
	QHBoxLayout* m_audioQualityLayout;
	QLabel* m_audioQualityLabel;
	AntRadioButton* m_audioQualityHighest;
	AntRadioButton* m_audioQuality320k;
	AntRadioButton* m_audioQuality256k;
	AntRadioButton* m_audioQuality192k;
	AntRadioButton* m_audioQuality128k;

	// 下载格式
	QHBoxLayout* m_downloadFormatLayout;
	QLabel* m_downloadFormatLabel;
	AntRadioButton* m_formatMerge;
	AntRadioButton* m_formatVideoOnly;
	AntRadioButton* m_formatAudioOnly;
	AntRadioButton* m_formatSeparate;

	// 同时下载数量
	QHBoxLayout* m_concurrentDownloadsLayout;
	QLabel* m_concurrentDownloadsLabel;
	AntRadioButton* m_concurrent1;
	AntRadioButton* m_concurrent2;
	AntRadioButton* m_concurrent3;
	AntRadioButton* m_concurrent4;
	AntRadioButton* m_concurrent5;

	// 网络设置
	QWidget* m_networkTab;
	QVBoxLayout* m_networkLayout;

	// 代理设置
	QHBoxLayout* m_proxyEnabledLayout;
	QLabel* m_proxyEnabledLabel;
	AntToggleButton* m_proxyEnabledToggle;

	QHBoxLayout* m_proxyTypeLayout;
	QLabel* m_proxyTypeLabel;
	AntComboBox* m_proxyTypeCombo;

	QHBoxLayout* m_proxyHostLayout;
	QLabel* m_proxyHostLabel;
	QLineEdit* m_proxyHostInput;

	QHBoxLayout* m_proxyPortLayout;
	QLabel* m_proxyPortLabel;
	QLineEdit* m_proxyPortInput;

	QHBoxLayout* m_proxyUserLayout;
	QLabel* m_proxyUserLabel;
	QLineEdit* m_proxyUserInput;

	QHBoxLayout* m_proxyPassLayout;
	QLabel* m_proxyPassLabel;
	QLineEdit* m_proxyPassInput;

	// 其他网络设置
	QHBoxLayout* m_timeoutLayout;
	QLabel* m_timeoutLabel;
	QLineEdit* m_timeoutInput;
	QHBoxLayout* m_retryCountLayout;
	QLabel* m_retryCountLabel;
	QLineEdit* m_retryCountInput;
	QHBoxLayout* m_userAgentLayout;
	QLabel* m_userAgentLabel;
	QLineEdit* m_userAgentInput;

	// 高级设置
	QWidget* m_advancedTab;
	QVBoxLayout* m_advancedLayout;
	QHBoxLayout* m_logPathLayout;
	QLabel* m_logPathLabel;
	QLineEdit* m_logPathInput;
	AntButton* m_browseLogPathButton;
	QHBoxLayout* m_logButtonsLayout;
	AntButton* m_viewLogsButton;
	AntButton* m_clearLogsButton;
};