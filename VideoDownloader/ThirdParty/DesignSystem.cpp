#include "DesignSystem.h"
#include <QApplication>
#include <QDir>

DesignSystem* DesignSystem::m_instance = nullptr;

DesignSystem::DesignSystem()
	: m_mode(Light)
{
	loadThemes();
	m_currentTheme = m_lightTheme;

	// 获取项目路径
	QString projectPath = QDir::currentPath();

	// 文件名
	QString fileName = "homeBg.mp4";

	// 默认路径，优先检查 项目根目录下的文件
	homeVideoPath = projectPath + "/QtAntDesign/Video/" + fileName;

	// 检查文件是否存在
	if (!QFile::exists(homeVideoPath))
	{
		// 如果文件不存在，检查工作目录下的 Video 文件夹
		homeVideoPath = QCoreApplication::applicationDirPath() + "/Video/" + fileName;
	}
}

void DesignSystem::loadThemes()
{
	// Light Theme
	m_lightTheme.primaryColor = QColor(22, 119, 255);	// Ant 蓝色
	m_lightTheme.primaryHoverColor = m_lightTheme.primaryColor.lighter(130);
	m_lightTheme.textColor = QColor(255, 255, 255);
	m_lightTheme.backgroundColor = QColor(255, 255, 255);
	m_lightTheme.widgetBgColor = QColor(240, 240, 240);
	m_lightTheme.widgetHoverBgColor = QColor(230, 230, 230);
	m_lightTheme.widgetSelectedBgColor = QColor(220, 220, 220);
	m_lightTheme.borderColor = QColor(200, 200, 200);
	m_lightTheme.borderColorHover = QColor(22, 119, 255);
	m_lightTheme.shadowColor = QColor(80, 80, 80);
	m_lightTheme.msgTextColor = QColor(30, 30, 30);
	m_lightTheme.msgBgColor = QColor(255, 255, 255);
	m_lightTheme.disabledColor = QColor(150, 150, 150);
	m_lightTheme.slideBgColor = QColor(200, 200, 200);
	m_lightTheme.slideColor = QColor(66, 133, 244);
	m_lightTheme.ringColor = QColor(153, 204, 255);
	m_lightTheme.ringhoveredColor = QColor(22, 119, 255);
	m_lightTheme.toggleButtonColor = QColor(255, 255, 255);
	m_lightTheme.toggleButtonBgColor = QColor(200, 200, 200);
	m_lightTheme.noCheckedColor = QColor(200, 200, 200);
	m_lightTheme.skeletonBgColor = QColor(220, 220, 220);
	m_lightTheme.toolTipBgColor = QColor(30, 30, 30);
	m_lightTheme.toolTipTextColor = QColor(245, 245, 245);
	m_lightTheme.userBubbleBgColor = QColor(255, 255, 255);
	m_lightTheme.toolBtnBgColor = QColor(24, 144, 255, 40);
	m_lightTheme.progressBarBgColor = QColor(224, 224, 224);
	m_lightTheme.tagTextColor = QColor(0, 0, 0);
	m_lightTheme.lineEditBorderColor = QColor(180, 180, 180);
	m_lightTheme.placeholderColor = QColor(150, 150, 150);
	m_lightTheme.checkBoxBgColor = QColor(255, 255, 255);
	m_lightTheme.checkBoxBorderEnableColor = QColor(180, 180, 180);
	m_lightTheme.checkBoxBorderDisableColor = QColor(200, 200, 200);
	m_lightTheme.checkBoxTextColor = QColor(0, 0, 0);
	m_lightTheme.checkBoxTextDisableColor = QColor(180, 180, 180);
	m_lightTheme.popupBgColor = QColor(255, 255, 255);
	m_lightTheme.popupBorderColor = QColor(200, 200, 200);
	m_lightTheme.popupScrollBarColor = QColor(220, 220, 220);
	m_lightTheme.popupItemBgColor = QColor(230, 230, 230, 180);
	m_lightTheme.popupTextColor = QColor(30, 30, 30);
	m_lightTheme.scrollAreaHandleColor = QColor(210, 210, 210);
	m_lightTheme.scrollAreaHoverColor = QColor(190, 190, 190);
	m_lightTheme.listTextColor = QColor(0, 0, 0);
	m_lightTheme.listItemDescColor = QColor(150, 150, 150);
	m_lightTheme.tableTextColor = QColor(0, 0, 0);
	m_lightTheme.tabBarBgColor = QColor(243, 243, 243);
	m_lightTheme.tabTextColor = QColor(30, 30, 30);
	m_lightTheme.tabHoverColor = QColor(230, 230, 230);
	m_lightTheme.onTabBtnHoverColor = QColor(200, 200, 200);
	m_lightTheme.tabContainerColor = QColor(200, 200, 200);
	m_lightTheme.notifBgColor = QColor(255, 255, 255);
	// 添加新的颜色定义
	m_lightTheme.cardBackgroundColor = QColor(255, 255, 255);
	m_lightTheme.primaryTextColor = QColor(0, 0, 0);
	m_lightTheme.secondaryTextColor = QColor(100, 100, 100);
	m_lightTheme.tertiaryTextColor = QColor(150, 150, 150);

	m_lightTheme.vipGradientStartColor = QColor("#a8c0ff");  // 浅蓝色，渐变起点，明亮且柔和
	m_lightTheme.vipGradientMidColor = QColor("#c6d1ff");    // 浅淡紫蓝，渐变过渡，自然
	m_lightTheme.vipGradientEndColor = QColor("#e2e8ff");    // 非常浅的蓝色，渐变终点，清爽柔和

	// Dark Theme
	m_darkTheme.primaryColor = QColor(243, 128, 100);           // 橙红色
	m_darkTheme.primaryHoverColor = m_darkTheme.primaryColor.lighter(130);

	m_darkTheme.textColor = QColor(255, 255, 255);				// 浅灰色文字，防止刺眼的纯白
	m_darkTheme.backgroundColor = QColor(45, 45, 45);           // 深色背景
	m_darkTheme.widgetBgColor = QColor(60, 60, 60);             // 控件背景色稍浅
	m_darkTheme.widgetHoverBgColor = QColor(80, 80, 80);        // 悬浮时更亮
	m_darkTheme.widgetSelectedBgColor = QColor(90, 90, 90);     // 选中状态更亮一些
	m_darkTheme.borderColor = QColor(120, 120, 120);
	m_darkTheme.borderColorHover = QColor(243, 128, 100);

	m_darkTheme.shadowColor = QColor(0, 0, 0, 0);

	m_darkTheme.msgTextColor = QColor(255, 255, 255);           // 消息文本颜色
	m_darkTheme.msgBgColor = QColor(100, 100, 100);
	m_darkTheme.disabledColor = QColor(200, 200, 200);          // 禁用颜色，灰度暗一些

	m_darkTheme.slideBgColor = QColor(120, 120, 120);
	m_darkTheme.slideColor = QColor(243, 128, 100);
	m_darkTheme.ringColor = QColor(243, 128, 100);
	m_darkTheme.ringhoveredColor = QColor(223, 100, 60);

	m_darkTheme.toggleButtonColor = QColor(255, 255, 255);
	m_darkTheme.toggleButtonBgColor = QColor(120, 120, 120);
	m_darkTheme.noCheckedColor = QColor(120, 120, 120);

	m_darkTheme.skeletonBgColor = QColor(120, 120, 120);

	m_darkTheme.toolTipBgColor = QColor(100, 100, 100);
	m_darkTheme.toolTipTextColor = QColor(250, 250, 250);

	m_darkTheme.userBubbleBgColor = QColor(100, 100, 100);

	m_darkTheme.toolBtnBgColor = QColor(243, 128, 100, 80);

	m_darkTheme.progressBarBgColor = QColor(120, 120, 120);

	m_darkTheme.tagTextColor = QColor(230, 230, 230);

	m_darkTheme.lineEditBorderColor = QColor(120, 120, 120);
	m_darkTheme.placeholderColor = QColor(180, 180, 180);

	m_darkTheme.checkBoxBgColor = QColor(120, 120, 120);
	m_darkTheme.checkBoxBorderEnableColor = QColor(120, 120, 120);
	m_darkTheme.checkBoxBorderDisableColor = QColor(80, 80, 80);
	m_darkTheme.checkBoxTextColor = QColor(230, 230, 230);
	m_darkTheme.checkBoxTextDisableColor = QColor(200, 200, 200);

	m_darkTheme.popupBgColor = QColor(60, 60, 60);
	m_darkTheme.popupBorderColor = QColor(120, 120, 120);
	m_darkTheme.popupScrollBarColor = QColor(100, 100, 100);
	m_darkTheme.popupItemBgColor = QColor(80, 80, 80, 180);
	m_darkTheme.popupTextColor = QColor(230, 230, 230);

	m_darkTheme.scrollAreaHandleColor = QColor(100, 100, 100);
	m_darkTheme.scrollAreaHoverColor = QColor(120, 120, 120);

	m_darkTheme.listTextColor = QColor(230, 230, 230);
	m_darkTheme.listItemDescColor = QColor(120, 120, 120);

	m_darkTheme.tableTextColor = QColor(230, 230, 230);

	m_darkTheme.tabBarBgColor = QColor(60, 60, 60);
	m_darkTheme.tabTextColor = QColor(230, 230, 230);
	m_darkTheme.tabHoverColor = QColor(80, 80, 80);
	m_darkTheme.onTabBtnHoverColor = QColor(100, 100, 100);
	m_darkTheme.tabContainerColor = QColor(120, 120, 120);

	m_darkTheme.notifBgColor = QColor(100, 100, 100);

	// 添加新的颜色定义
	m_darkTheme.cardBackgroundColor = QColor(60, 60, 60);
	m_darkTheme.primaryTextColor = QColor(255, 255, 255);
	m_darkTheme.secondaryTextColor = QColor(200, 200, 200);
	m_darkTheme.tertiaryTextColor = QColor(150, 150, 150);

	m_darkTheme.vipGradientStartColor = QColor("#2a2e48");   // 深蓝紫，渐变起点，稳重且暗
	m_darkTheme.vipGradientMidColor = QColor("#3f4268");     // 中间蓝灰，过渡自然
	m_darkTheme.vipGradientEndColor = QColor("#5a5f8a");     // 浅蓝紫，渐变终点，柔和且不刺眼
}

void DesignSystem::setThemeMode(ThemeMode mode)
{
	m_mode = mode;
	m_currentTheme = (mode == Light) ? m_lightTheme : m_darkTheme;
}

DesignSystem::ThemeMode DesignSystem::themeMode() const
{
	return m_mode;
}

void DesignSystem::switchTheme()
{
	if (m_mode == Light)
	{
		m_mode = Dark;
	}
	else
	{
		m_mode = Light;
	}
	setThemeMode(m_mode);
}

QIcon& DesignSystem::setThemeIcon()
{
	if (m_mode == Light)
	{
		m_themeIcon = QIcon(":/Imgs/sun.svg");
	}
	else
	{
		m_themeIcon = QIcon(":/Imgs/moon.svg");
	}
	return m_themeIcon;
}

void DesignSystem::setTransparentMask(TransparentMask* tpMask)
{
	m_tpMask = tpMask;
}

TransparentMask* DesignSystem::getTransparentMask()
{
	return m_tpMask;
}

void DesignSystem::setDarkMask(MaskWidget* mask)
{
	m_darkMask = mask;
}

MaskWidget* DesignSystem::getDarkMask()
{
	return m_darkMask;
}

QIcon& DesignSystem::btnMinIcon()
{
	if (m_mode == Light)
	{
		btnMin = QIcon(":/Imgs/Minimize.svg");
	}
	else
	{
		btnMin = QIcon(":/Imgs/MinimizeDark.svg");
	}

	return btnMin;
}

QIcon& DesignSystem::btnMaxIcon()
{
	if (m_mode == Light)
	{
		btnMax = QIcon(":/Imgs/Maximize.svg");
	}
	else
	{
		btnMax = QIcon(":/Imgs/MaximizeDark.svg");
	}

	return btnMax;
}

QIcon& DesignSystem::btnCloseIcon()
{
	if (m_mode == Light)
	{
		btnClose = QIcon(":/Imgs/ShutDown.svg");
	}
	else
	{
		btnClose = QIcon(":/Imgs/ShutDownDark.svg");
	}

	return btnClose;
}

QIcon& DesignSystem::btnRestoreIcon()
{
	if (m_mode == Light)
	{
		btnRestore = QIcon(":/Imgs/Restore.svg");
	}
	else
	{
		btnRestore = QIcon(":/Imgs/RestoreDark.svg");
	}

	return btnRestore;
}

QString& DesignSystem::btnHomeIconPath()
{
	if (m_mode == Light)
	{
		btnHome = ":/Imgs/home.svg";
	}
	else
	{
		btnHome = ":/Imgs/homeDark.svg";
	}

	return btnHome;
}

QString& DesignSystem::btnDownloadIconPath()
{
	if (m_mode == Light)
	{
		btnDownload = ":/Imgs/function.svg";
	}
	else
	{
		btnDownload = ":/Imgs/functionDark.svg";
	}

	return btnDownload;
}

QString& DesignSystem::btnModManagerIconPath()
{
	if (m_mode == Light)
	{
		btnModManager = ":/Imgs/about.svg";
	}
	else
	{
		btnModManager = ":/Imgs/aboutDark.svg";
	}

	return btnModManager;
}

QString& DesignSystem::btnSettingsIconPath()
{
	if (m_mode == Light)
	{
		btnSettings = ":/Imgs/settings.svg";
	}
	else
	{
		btnSettings = ":/Imgs/settingsDark.svg";
	}

	return btnSettings;
}

QString& DesignSystem::btnHomeActiveIconPath()
{
	if (m_mode == Light)
	{
		btnHomeActive = ":/Imgs/home_active.svg";
	}
	else
	{
		btnHomeActive = ":/Imgs/home_activeDark.svg";
	}

	return btnHomeActive;
}

QString& DesignSystem::btnDownloadActiveIconPath()
{
	if (m_mode == Light)
	{
		btnDownload = ":/Imgs/function_active.svg";
	}
	else
	{
		btnDownload = ":/Imgs/functionDark_active.svg";
	}

	return btnDownload;
}

QString& DesignSystem::btnModManagerActiveIconPath()
{
	if (m_mode == Light)
	{
		btnModManager = ":/Imgs/about_active.svg";
	}
	else
	{
		btnModManager = ":/Imgs/about_activeDark.svg";
	}

	return btnModManager;
}

QString& DesignSystem::btnSettingsActiveIconPath()
{
	if (m_mode == Light)
	{
		btnSettingsActive = ":/Imgs/settings_active.svg";
	}
	else
	{
		btnSettingsActive = ":/Imgs/settings_activeDark.svg";
	}

	return btnSettingsActive;
}

QString& DesignSystem::prevBtnIcon()
{
	if (m_mode == Light)
	{
		prevBtn = ":/Imgs/PreviousBtn.svg";
	}
	else
	{
		prevBtn = ":/Imgs/PreviousBtnDark.svg";
	}

	return prevBtn;
}

QString& DesignSystem::nextBtnIcon()
{
	if (m_mode == Light)
	{
		nextBtn = ":/Imgs/nextBtn.svg";
	}
	else
	{
		nextBtn = ":/Imgs/nextBtnDark.svg";
	}

	return nextBtn;
}

QString& DesignSystem::prevBtnDisableIcon()
{
	if (m_mode == Light)
	{
		prevDisBtn = ":/Imgs/previousBtnDisable.svg";
	}
	else
	{
		prevDisBtn = ":/Imgs/previousBtnDisableDark.svg";
	}

	return prevDisBtn;
}

QString& DesignSystem::nextBtnDisableIcon()
{
	if (m_mode == Light)
	{
		nextDisBtn = ":/Imgs/nextBtnDisable.svg";
	}
	else
	{
		nextDisBtn = ":/Imgs/nextBtnDisableDark.svg";
	}

	return nextDisBtn;
}

QString& DesignSystem::homeVideoFilePath()
{
	return homeVideoPath;
}

void DesignSystem::setContentSize(QSize size)
{
	m_contentSzie = size;
}

QSize DesignSystem::contentSize() const
{
	return m_contentSzie;
}

const Theme& DesignSystem::currentTheme() const
{
	return m_currentTheme;
}

QColor DesignSystem::primaryColor() const
{
	return m_currentTheme.primaryColor;
}

QColor DesignSystem::textColor() const
{
	return m_currentTheme.textColor;
}

QColor DesignSystem::backgroundColor() const
{
	return m_currentTheme.backgroundColor;
}

QColor DesignSystem::widgetBgColor() const
{
	return m_currentTheme.widgetBgColor;
}

QColor DesignSystem::widgetHoverBgColor() const
{
	return m_currentTheme.widgetHoverBgColor;
}

QColor DesignSystem::borderColor() const
{
	return m_currentTheme.borderColor;
}

QColor DesignSystem::borderColorHover() const
{
	return m_currentTheme.borderColorHover;
}

QColor DesignSystem::disabledColor() const
{
	return m_currentTheme.disabledColor;
}