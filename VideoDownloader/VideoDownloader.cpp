#include "VideoDownloader.h"
#include <qevent.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qscreen.h>
#include "StyleSheet.h"
#include "AntMessageManager.h"
#include "AntTooltipManager.h"
#include "DialogViewController.h"
#include "HomePage.h"
#include "DownloadPage.h"
#include "ModManagerPage.h"
#include "SettingsPage.h"
#include <QToolButton>
#include <QLabel>
#include <QShowEvent>
#include <QWindow>
#include "NotificationManager.h"
#include "DesignSystem.h"
#include "TransparentMask.h"
#include "MaskWidget.h"
#include "ThemeSwitcher.h"

VideoDownloader::VideoDownloader(QWidget* parent)
	: QWidget(parent)
	, appController(QSharedPointer<ApplicationController>::create())
	, downloadManager(QSharedPointer<DownloadManager>::create(appController))
{
	ui.setupUi(this);

	setObjectName("QtAntDesign");
	#ifdef Q_OS_LINUX
	setWindowFlags(Qt::FramelessWindowHint);
	// 开启悬浮事件处理鼠标边界样式变化
	setAttribute(Qt::WA_Hover, true);
	installEventFilter(this);
	#endif

	#ifdef Q_OS_WIN
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_NoSystemBackground);
	#endif
	QSize miniSize(1120, 725);
	setMinimumSize(miniSize);

	ui.main_widget->setStyleSheet(StyleSheet::mainQss(DesignSystem::instance()->backgroundColor()));

	// 获取主屏幕尺寸
	int w = 0, h = 0;
	QScreen* screen = QGuiApplication::primaryScreen();
	if (screen)
	{
		QSize screenSize = screen->availableSize();		// 可用屏幕大小，不包括任务栏
		w = int(screenSize.width() * 0.50);  // % 宽度
		h = int(screenSize.height() * 0.60); // % 高度
		if (w < miniSize.width() && h < miniSize.height())
		{
			w = miniSize.width();
			h = miniSize.height();
		}
		resize(w, h);
	}
	setContentsMargins(0, 0, 0, 0);

	// 初始化全局设计系统 必须写在最前面 因为它会设置主题和主窗口指针注册一些全局变量
	DesignSystem::instance()->setThemeMode(DesignSystem::Light);					// 默认亮主题
	DesignSystem::instance()->setMainWindow(this);									// 获取主窗口指针
	ThemeSwitcher* themeSwitcher = new ThemeSwitcher(this);
	themeSwitcher->setThemeColor();
	// 注册全局透明遮罩
	TransparentMask* tpMask = new TransparentMask(this);
	DesignSystem::instance()->setTransparentMask(tpMask);
	// 全局深色动画遮罩
	MaskWidget* darkMask = new MaskWidget(w, h, this);
	DesignSystem::instance()->setDarkMask(darkMask);

	// 任务栏 内容区域 导航栏 布局调整
	ui.navi_widget->setFixedWidth(m_naviWidth); // 希望的宽度
	ui.navi_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);	// 水平方向缩放策略固定
	ui.navi_widget->setStyleSheet(StyleSheet::naviQss(DesignSystem::instance()->widgetBgColor()));
	ui.titleBar->setStyleSheet(StyleSheet::titleBarQss());
	ui.central->setStyleSheet(StyleSheet::centralQss());
	//ui.titleBar->setFixedHeight(0);	//首页隐藏标题栏 实现沉浸感

	// 标题栏
	QHBoxLayout* titleLay = new QHBoxLayout(ui.titleBar);
	titleLay->setContentsMargins(20, 0, rightMargin, 0);
	titleLay->setSpacing(titleBarSpacing);
	QFont font;
	font.setPointSizeF(16);
	font.setBold(true);
	QLabel* title = new QLabel("视频下载器", ui.titleBar);
	title->setFont(font);
	// 创建按钮
	btnMin = new QToolButton(ui.titleBar);
	btnMax = new QToolButton(ui.titleBar);
	btnClose = new QToolButton(ui.titleBar);
	// 设置图标
	btnMin->setIcon(DesignSystem::instance()->btnMinIcon());
	btnMax->setIcon(DesignSystem::instance()->btnMaxIcon());
	btnClose->setIcon(DesignSystem::instance()->btnCloseIcon());
	// 设置QSS
	btnMin->setStyleSheet(StyleSheet::toolBtnQss());
	btnMax->setStyleSheet(StyleSheet::toolBtnQss());
	btnClose->setStyleSheet(StyleSheet::toolBtnQss());
	// 设置固定大小（根据图标适当调整）
	btnMin->setFixedSize(32, 32);
	btnMax->setFixedSize(32, 32);
	btnClose->setFixedSize(34, 34);
	int extraWidth = 100;	// 额外的间距宽度: 是控件间距以及标题栏两端的间距 自己根据标题栏所有控件的宽度调整
	m_widgetTotalWidth = btnMin->width() + btnMax->width() + btnClose->width();

	// 标题栏右侧所有控件的长宽转为物理像素后在native事件中限制标题栏的范围
	qreal dpiScale = QApplication::primaryScreen()->devicePixelRatio();
	// 计算控件总宽度（逻辑像素） 右侧4个控件3个间隔
	int widgetTotalWidth = rightMargin + btnMin->width() + btnMax->width() + btnClose->width() + 2 * titleBarSpacing;
	// 转换为物理像素
	m_widgetTotalWidthPhysicalPixels = static_cast<int>(widgetTotalWidth * dpiScale);
	// 同理转换标题栏左侧
	m_titleLeftTotalWidthPhysicalPixels = static_cast<int>(m_naviWidth * dpiScale);
	// 同理转换标题栏高度
	m_titleBarHeightPhysicalPixels = static_cast<int>(m_titleBarHeight * dpiScale);

	// 将标题和按钮添加到布局
	titleLay->addWidget(title);
	titleLay->addStretch();
	titleLay->addWidget(btnMin);
	titleLay->addWidget(btnMax);
	titleLay->addWidget(btnClose);
	totalSpacingWidth = 2 * titleBarSpacing;	// 标题栏右侧3个控件中间2个间隔

	// 导航栏添加控件
	QVBoxLayout* naviLay = new QVBoxLayout(ui.navi_widget);;
	ui.navi_widget->layout()->setContentsMargins(0, 0, 0, 0);
	CircularAvatar* avatar = new CircularAvatar(QSize(42, 42), ":/Imgs/noLogin.svg", ":/Imgs/github.svg", ui.navi_widget);
	// 添加页面布局
	QVBoxLayout* contentLay = new QVBoxLayout(ui.central);
	contentLay->setContentsMargins(0, 0, 0, 0);
	contentLay->setSpacing(0);
	stackedWidget = new SlideStackedWidget(ui.central);
	contentLay->addWidget(stackedWidget);
	// 添加页面
	HomePage* homePage = new HomePage(appController, stackedWidget);
	DownloadPage* downloadPage = new DownloadPage(downloadManager, stackedWidget);

	connect(homePage, &HomePage::navigateToDownloadRequested, this, [this, downloadPage]() {
		// 切换到下载页面
		stackedWidget->setCurrentWidget(downloadPage);

		// 设置下载页面的标签页为"待下载"
		if (downloadPage) {
			// 这里需要根据你的DownloadPage实现来设置当前标签页
			// 例如：downloadPage->setCurrentTab(0);
		}

		// 更新导航按钮状态
		for (ButtonInfo& info : buttonInfos) {
			if (info.page == downloadPage) {
				info.button->setBtnChecked(true);
			}
			else {
				info.button->setBtnChecked(false);
			}
		}

		qDebug() << "成功跳转到下载页面";
		});


	ModManagerPage* modManagerPage = new ModManagerPage(stackedWidget);
	SettingsPage* settingsPage = new SettingsPage(appController, stackedWidget);

	stackedWidget->addWidget(homePage);
	stackedWidget->addWidget(downloadPage);
	stackedWidget->addWidget(modManagerPage);
	stackedWidget->addWidget(settingsPage);
	stackedWidget->setCurrentIndex(0);				// 默认显示首页
	// 导航按钮
	const int naviWidth = ui.navi_widget->width();  // 动态获取导航栏宽度
	const double iconSizeRatio = 0.56;				// 图标占按钮的比例
	const int buttonSize = naviWidth;
	const int iconSize = static_cast<int>(buttonSize * iconSizeRatio);
	CustomToolButton* btnHome = new CustomToolButton(QSize(iconSize, iconSize), ui.navi_widget);
	CustomToolButton* btnDownload = new CustomToolButton(QSize(iconSize, iconSize), ui.navi_widget);
	CustomToolButton* btnModManager = new CustomToolButton(QSize(iconSize, iconSize), ui.navi_widget);
	CustomToolButton* btnSettings = new CustomToolButton(QSize(iconSize, iconSize), ui.navi_widget);
	// 设置导航按钮样式
	auto* ins = DesignSystem::instance();
	buttonInfos = {
	{btnHome,ins->btnHomeIconPath(), ins->btnHomeActiveIconPath(), homePage},
	{btnDownload, ins->btnDownloadIconPath(), ins->btnDownloadActiveIconPath(), downloadPage},
	{btnModManager,ins->btnModManagerIconPath() ,ins->btnModManagerActiveIconPath() , modManagerPage},
	{btnSettings,ins->btnSettingsIconPath() ,ins->btnSettingsActiveIconPath() , settingsPage}
	};
	// 设置统一样式和连接信号
	buttonInfos[stackedWidget->currentIndex()].button->setBtnChecked(true); // 设置当前页面按钮为选中状态
	for (const ButtonInfo& info : buttonInfos)
	{
		CustomToolButton* btn = info.button;
		btn->setSvgIcons(info.normalIcon, info.activeIcon);
		btn->setFixedSize(QSize(naviWidth - 4, naviWidth - 4));
		// 连接信号，捕获图标路径
		connect(btn, &QToolButton::clicked, [btn, btnHome, contentLay, this]()
			{
				//if (btn == btnHome)
				//{
				//	ui.titleBar->setFixedHeight(0);
				//}
				//else
				//{
				ui.titleBar->setFixedHeight(m_titleBarHeight);
				//}

				// 禁用所有按钮，防止视觉反馈+误点
				for (ButtonInfo& infos : buttonInfos)
					infos.button->setEnabled(false);

				if (stackedWidget->isAnimationRunning()) return;

				// 更新UI
				QWidget* nextPage = nullptr;
				btn->setBtnChecked(true);
				for (ButtonInfo& infos : buttonInfos)
				{
					if (infos.button == btn)
						nextPage = infos.page;
					else
						infos.button->setBtnChecked(false);
				}

				// 页面切换 + 结束后恢复按钮
				stackedWidget->slideFromBottomToTop(nextPage, 250, [this]()
					{
						for (ButtonInfo& infos : buttonInfos)
							infos.button->setEnabled(true);
					});
			});
	}
	// 主题切换按钮
	QPushButton* themeBtn = new QPushButton(ui.navi_widget);
	themeBtn->setStyleSheet("QPushButton { background-color: transparent; border: none; }");
	themeBtn->setFixedSize(naviWidth * 0.38, naviWidth * 0.38);
	themeBtn->setIcon(DesignSystem::instance()->setThemeIcon());
	themeBtn->setIconSize(themeBtn->size());
	connect(themeBtn, &QPushButton::clicked, this, [this, themeSwitcher, themeBtn]()
		{
			// 获取按钮中心点的全局坐标
			themeSwitcher->startSwitchTheme(this->grab(), themeBtn, themeBtn->mapToGlobal(themeBtn->rect().center()));
		});
	connect(this, &VideoDownloader::resized, themeSwitcher, &ThemeSwitcher::resizeByMainWindow);

	//  导航栏布局
	naviLay->addSpacing(28);
	naviLay->addWidget(avatar, 0, Qt::AlignHCenter);
	naviLay->addSpacing(16);
	naviLay->addWidget(btnHome, 0, Qt::AlignHCenter);
	naviLay->addWidget(btnDownload, 0, Qt::AlignHCenter);
	naviLay->addWidget(btnModManager, 0, Qt::AlignHCenter);
	naviLay->addStretch();
	naviLay->addWidget(themeBtn, 0, Qt::AlignHCenter);
	naviLay->addWidget(btnSettings, 0, Qt::AlignHCenter);
	naviLay->addSpacing(10);

	// 初始化全局管理器
	AntMessageManager::instance();	// 全局消息
	AntTooltipManager::instance();	// 全局提示

	// 初始化通知管理器
	NotificationManager::instance()->getMainWindow(ui.main_widget);

	// 调整消息框位置
	connect(this, &VideoDownloader::resized, this, [=](int w, int h)
		{
			NotificationManager::instance()->relayoutNotifications(w, h);
		});

	// 对话框
	DialogViewController* mDialog = new DialogViewController(avatar->loginState(), this);	// 实际登录状态要服务器给予
	avatar->addDialog(mDialog);
	connect(mDialog, &DialogViewController::successLogin, avatar, &CircularAvatar::allowLogin);

	// 信号连接
	connect(btnMax, &QToolButton::clicked, this, [this]()
		{
			#ifdef Q_OS_WIN
			// 每次使用时重新获取窗口句柄，可以确保你拿到的是当前最新、有效的窗口句柄。避免使用缓存失效的句柄造成崩溃。
			HWND hwnd = reinterpret_cast<HWND>(winId());

			// 如果当前最大化，点击恢复；否则最大化
			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hwnd, &wp);

			if (wp.showCmd == SW_MAXIMIZE)
			{
				ShowWindow(hwnd, SW_RESTORE);
			}
			else
			{
				ShowWindow(hwnd, SW_MAXIMIZE);
			}
			#endif

			#ifdef Q_OS_LINUX
			if (isMaximized()) showNormal();
			else showMaximized();
			#endif
		});

	connect(btnMin, &QToolButton::clicked, this, [this]()
		{
			#ifdef Q_OS_WIN
			HWND hwnd = reinterpret_cast<HWND>(winId());
			ShowWindow(hwnd, SW_MINIMIZE);
			#endif

			#ifdef Q_OS_LINUX
			showMinimized();
			#endif
		});

	connect(btnClose, &QToolButton::clicked, this, [this, w, h]()
		{
			emit showStandardDialog("TITLE", "是否关闭应用程序?");
		});

	connect(this, &VideoDownloader::showStandardDialog, mDialog, &DialogViewController::buildStandardDialog);

	// 页面的信号连接
	connect(this, &VideoDownloader::resized, downloadPage, &DownloadPage::resized);
	connect(this, &VideoDownloader::resized, this, [this](int w, int h)
		{
			DesignSystem::instance()->getDarkMask()->resize(w, h);
		});
	connect(this, &VideoDownloader::windowMoved, downloadPage, &DownloadPage::windowMoved);

	// 主题切换
	connect(ins, &DesignSystem::themeChanged, this, [=]()
		{
			ui.main_widget->setStyleSheet(StyleSheet::mainQss(DesignSystem::instance()->backgroundColor()));
			ui.navi_widget->setStyleSheet(StyleSheet::naviQss(DesignSystem::instance()->widgetBgColor()));
			ui.titleBar->setStyleSheet(StyleSheet::titleBarQss());
			btnMin->setIcon(DesignSystem::instance()->btnMinIcon());
			btnMax->setIcon(DesignSystem::instance()->btnMaxIcon());
			btnClose->setIcon(DesignSystem::instance()->btnCloseIcon());
			buttonInfos = {
				{btnHome,ins->btnHomeIconPath(), ins->btnHomeActiveIconPath(), homePage},
				{btnDownload, ins->btnDownloadIconPath(), ins->btnDownloadActiveIconPath(), downloadPage},
				{btnModManager, ins->btnModManagerIconPath(), ins->btnModManagerActiveIconPath(), modManagerPage},
				{btnSettings,ins->btnSettingsIconPath() ,ins->btnSettingsActiveIconPath() , settingsPage}
			};
			for (const ButtonInfo& info : buttonInfos)
			{
				CustomToolButton* btn = info.button;
				btn->setSvgIcons(info.normalIcon, info.activeIcon);
			}
		});
}

VideoDownloader::~VideoDownloader()
{
}

void VideoDownloader::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	// 调整对话框尺寸
	emit resized(width(), height());

	// 保存内容区域尺寸
	DesignSystem::instance()->setContentSize(QSize(width() - m_naviWidth, height()));
}

bool VideoDownloader::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
	#ifdef Q_OS_WIN
	MSG* msg = static_cast<MSG*>(message);

	switch (msg->message) {
	case WM_NCCALCSIZE: {
		// 去掉非客户区，让客户区占满整个窗口（无边框）
		if (msg->wParam) {
			*result = 0;
			return true;
		}
		break;
	}
	case WM_NCLBUTTONDBLCLK: {
		// 如果存在遮罩则响应遮罩，禁止系统事件（双击最大化）
		if (DesignSystem::instance()->getTransparentMask()->isVisible() ||
			DesignSystem::instance()->getDarkMask()->isVisible())
		{
			*result = 0;  // 阻止系统响应双击
			return true;
		}
		break;
	}
	case WM_NCHITTEST: {
		if (DesignSystem::instance()->getTransparentMask()->isVisible() ||
			DesignSystem::instance()->getDarkMask()->isVisible())
		{
			*result = HTCLIENT;  // 阻止系统响应拖拽缩放
			return true;
		}

		// 处理拖拽和缩放区域
		const LONG borderWidth = 8; // 拖拽缩放边框宽度

		// 返回的是物理像素
		RECT winRect;
		GetWindowRect(HWND(winId()), &winRect);

		// 获取鼠标全局坐标物理像素
		const LONG x = GET_X_LPARAM(msg->lParam);
		const LONG y = GET_Y_LPARAM(msg->lParam);

		// 允许缩放的条件（根据窗口最小最大宽高判断）
		const bool canResizeWidth = minimumWidth() != maximumWidth();
		const bool canResizeHeight = minimumHeight() != maximumHeight();

		// 左上角
		if (canResizeWidth && canResizeHeight &&
			x >= winRect.left && x < winRect.left + borderWidth &&
			y >= winRect.top && y < winRect.top + borderWidth) {
			*result = HTTOPLEFT;
			return true;
		}
		// 右上角
		if (canResizeWidth && canResizeHeight &&
			x >= winRect.right - borderWidth && x < winRect.right &&
			y >= winRect.top && y < winRect.top + borderWidth) {
			*result = HTTOPRIGHT;
			return true;
		}
		// 左下角
		if (canResizeWidth && canResizeHeight &&
			x >= winRect.left && x < winRect.left + borderWidth &&
			y >= winRect.bottom - borderWidth && y < winRect.bottom) {
			*result = HTBOTTOMLEFT;
			return true;
		}
		// 右下角
		if (canResizeWidth && canResizeHeight &&
			x >= winRect.right - borderWidth && x < winRect.right &&
			y >= winRect.bottom - borderWidth && y < winRect.bottom) {
			*result = HTBOTTOMRIGHT;
			return true;
		}
		// 左边
		if (canResizeWidth &&
			x >= winRect.left && x < winRect.left + borderWidth) {
			*result = HTLEFT;
			return true;
		}
		// 右边
		if (canResizeWidth &&
			x >= winRect.right - borderWidth && x < winRect.right) {
			*result = HTRIGHT;
			return true;
		}
		// 上边
		if (canResizeHeight &&
			y >= winRect.top && y < winRect.top + borderWidth) {
			*result = HTTOP;
			return true;
		}
		// 下边
		if (canResizeHeight &&
			y >= winRect.bottom - borderWidth && y < winRect.bottom) {
			*result = HTBOTTOM;
			return true;
		}
		// 如果当前是首页, 标题栏隐藏, 因此不需要带上标题栏右侧全部控件的宽度 重置系统标题栏拖动区域
		//bool isHomePage = stackedWidget->currentIndex() == 0; // 判断是否是首页
		int rightBoundary = false ? winRect.right : winRect.right - m_widgetTotalWidthPhysicalPixels;
		// 设置标题栏拖动区域 只有该区域内才允许拖动窗口
		if (x > winRect.left + m_titleLeftTotalWidthPhysicalPixels && x < rightBoundary
			&& y > winRect.top && y < winRect.top + m_titleBarHeightPhysicalPixels)
		{
			*result = HTCAPTION;
			return true;
		}
		// 其余地方交给默认处理
		break;
	}
	default:
		break;
	}
	#endif // Q_OS_WIN
	return QWidget::nativeEvent(eventType, message, result);
}

void VideoDownloader::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);

	#ifdef Q_OS_WIN
	m_hwnd = reinterpret_cast<HWND>(winId());
	if (!m_hwnd) return;

	LONG style = GetWindowLong(m_hwnd, GWL_STYLE);

	// 非客户区被隐藏了 但是他还有标题栏、有边框以及最大化最小化功能的标准窗口从而提供windows原生动画和交互
	style |= WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	SetWindowLong(m_hwnd, GWL_STYLE, style);

	DWORD cornerPref = 2;
	DwmSetWindowAttribute(m_hwnd, 33, &cornerPref, sizeof(cornerPref));
	#endif // Q_OS_WIN
}

void VideoDownloader::moveEvent(QMoveEvent* event)
{
	QWidget::moveEvent(event);
	// 发送窗口左上角全局坐标
	emit windowMoved(this->mapToGlobal(QPoint(0, 0)));
}

void VideoDownloader::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::WindowStateChange)
	{
		QWindowStateChangeEvent* stateEvent = static_cast<QWindowStateChangeEvent*>(event);

		if (isMaximized())
		{
			btnMax->setIcon(DesignSystem::instance()->btnRestoreIcon());
			ui.main_widget->layout()->setContentsMargins(0, 0, 5, 0);
		}
		else if (stateEvent->oldState() & Qt::WindowMaximized)
		{
			btnMax->setIcon(DesignSystem::instance()->btnMaxIcon());
			ui.main_widget->layout()->setContentsMargins(0, 0, 0, 0);
		}
	}

	QWidget::changeEvent(event);
}

#ifdef Q_OS_LINUX

bool VideoDownloader::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::HoverMove && !m_isLockCursor)
	{
		QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
		QPoint pos = hoverEvent->position().toPoint();
		updateCursor(pos);
		return true; // 表示事件已处理
	}
	if (event->type() == QEvent::MouseButtonRelease)
	{
		QMouseEvent* me = static_cast<QMouseEvent*>(event);
		m_isLockCursor = false; // 释放鼠标锁定状态
		return true;
	}

	return QObject::eventFilter(obj, event);
}

void VideoDownloader::mousePressEvent(QMouseEvent* event)
{
	if (event->button() != Qt::LeftButton) return;

	m_isLockCursor = true;
	updateCursor(event->pos());

	if (currentEdge != Qt::Edges())
	{
		// 鼠标在窗口边缘/角落，启动系统缩放
		window()->windowHandle()->startSystemResize(currentEdge);
	}
	else if (event->pos().x() > rect().left() + m_naviWidth && event->pos().x() < rect().right()
		&& event->pos().y() > rect().top() && event->pos().y() < m_titleBarHeight)
	{
		// 鼠标在标题栏，拖动窗口
		window()->windowHandle()->startSystemMove();
	}
}

void VideoDownloader::mouseDoubleClickEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && event->pos().y() > rect().top() && event->pos().y() < m_titleBarHeight)
	{
		if (isMaximized()) showNormal();
		else showMaximized();
	}
}

void VideoDownloader::updateCursor(const QPoint& pos)
{
	QRectF r = rect();
	Qt::Edges newEdge = Qt::Edges();

	const bool left = pos.x() < r.left() + edgeWidth;
	const bool right = pos.x() > r.right() - edgeWidth;
	const bool top = pos.y() < r.top() + edgeWidth;
	const bool bottom = pos.y() > r.bottom() - edgeWidth;

	// 四角
	if (top && left)        newEdge = Qt::TopEdge | Qt::LeftEdge;
	else if (top && right)  newEdge = Qt::TopEdge | Qt::RightEdge;
	else if (bottom && left) newEdge = Qt::BottomEdge | Qt::LeftEdge;
	else if (bottom && right) newEdge = Qt::BottomEdge | Qt::RightEdge;
	// 边
	else if (top)    newEdge = Qt::TopEdge;
	else if (bottom) newEdge = Qt::BottomEdge;
	else if (left)   newEdge = Qt::LeftEdge;
	else if (right)  newEdge = Qt::RightEdge;

	// 避免重复 setCursor
	if (newEdge != currentEdge)
	{
		currentEdge = newEdge;

		switch (newEdge)
		{
		case Qt::TopEdge | Qt::LeftEdge:
		case Qt::BottomEdge | Qt::RightEdge:
			setCursor(Qt::SizeFDiagCursor);
			break;
		case Qt::TopEdge | Qt::RightEdge:
		case Qt::BottomEdge | Qt::LeftEdge:
			setCursor(Qt::SizeBDiagCursor);
			break;
		case Qt::TopEdge:
		case Qt::BottomEdge:
			setCursor(Qt::SizeVerCursor);
			break;
		case Qt::LeftEdge:
		case Qt::RightEdge:
			setCursor(Qt::SizeHorCursor);
			break;
		default:
			setCursor(Qt::ArrowCursor);
			break;
		}
	}
}

#endif