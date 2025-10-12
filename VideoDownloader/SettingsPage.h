#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QDesktopServices>
#include <QApplication>
#include "ApplicationController.h"
#include "ConfigManager.h"
#include "DesignSystem.h"
#include "AntButton.h"
#include "AntToggleButton.h"
#include "AntSlider.h"
#include "MaterialProgressBar.h"
#include "LogSystem.h"

class SettingsPage : public QWidget
{
	Q_OBJECT

public:
	SettingsPage(QSharedPointer<ApplicationController> appController, QWidget* parent = nullptr);
	~SettingsPage();

private slots:
	void onSaveSettings();
	void onResetSettings();
	void onThemeChanged();
	void onDownloadPathBrowse();
	void onProxySettingsChanged();
	void onLogLevelChanged(int index);
	void onMaxConcurrentDownloadsChanged(int value);
	void onModEnabledToggled(bool enabled);

private:
	void setupUI();
	void setupConnections();
	void loadCurrentSettings();
	void saveCurrentSettings();
	void applyTheme();

	// 设置类别
	void setupGeneralSettings();
	void setupDownloadSettings();
	void setupNetworkSettings();
	void setupUISettings();
	void setupModSettings();
	void setupAdvancedSettings();

	QSharedPointer<ApplicationController> m_appController;
	QSharedPointer<ConfigManager> m_configManager;

	// 主布局
	QVBoxLayout* m_mainLayout;
	QTabWidget* m_tabWidget;

	// 常规设置
	QWidget* m_generalTab;
	QLineEdit* m_appNameInput;
	QLineEdit* m_organizationInput;
	AntToggleButton* m_autoStartToggle;
	AntToggleButton* m_checkUpdatesToggle;
	AntToggleButton* m_minimizeToTrayToggle;

	// 下载设置
	QWidget* m_downloadTab;
	QLineEdit* m_downloadPathInput;
	AntButton* m_browsePathButton;
	QComboBox* m_videoQualityCombo;
	QComboBox* m_audioQualityCombo;
	QComboBox* m_downloadFormatCombo;
	AntSlider* m_maxConcurrentSlider;
	QLabel* m_maxConcurrentLabel;
	AntToggleButton* m_autoMergeToggle;
	AntToggleButton* m_autoDeleteTempToggle;

	// 网络设置
	QWidget* m_networkTab;
	AntToggleButton* m_proxyEnabledToggle;
	QComboBox* m_proxyTypeCombo;
	QLineEdit* m_proxyHostInput;
	QLineEdit* m_proxyPortInput;
	QLineEdit* m_proxyUserInput;
	QLineEdit* m_proxyPassInput;
	QSpinBox* m_timeoutInput;
	QSpinBox* m_retryCountInput;
	QLineEdit* m_userAgentInput;

	// 界面设置
	QWidget* m_uiTab;
	QComboBox* m_themeCombo;
	QComboBox* m_languageCombo;
	QComboBox* m_startupPageCombo;
	AntToggleButton* m_showTrayIconToggle;
	AntToggleButton* m_closeToTrayToggle;
	QComboBox* m_fontSizeCombo;

	// Mod设置
	QWidget* m_modTab;
	QVBoxLayout* m_modListLayout;
	AntToggleButton* m_autoUpdateModsToggle;
	AntButton* m_refreshModsButton;
	AntButton* m_openModsFolderButton;

	// 高级设置
	QWidget* m_advancedTab;
	QComboBox* m_logLevelCombo;
	QLineEdit* m_logPathInput;
	AntButton* m_browseLogPathButton;
	AntButton* m_viewLogsButton;
	AntButton* m_clearLogsButton;
	AntToggleButton* m_debugModeToggle;
	AntButton* m_resetAllSettingsButton;

	// 底部按钮
	QHBoxLayout* m_buttonLayout;
	AntButton* m_saveButton;
	AntButton* m_resetButton;
	AntButton* m_cancelButton;
};