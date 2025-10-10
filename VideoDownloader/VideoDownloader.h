#pragma once

#include <QtWidgets/QWidget>
#include "ui_QtAntDesign.h"
#include <QHoverEvent>
#include "CircularAvatar.h"
#include "CustomToolButton.h"
#include <QLabel>
#include <QTimer>
#include <QEvent>
#include <QSharedPointer>
#include "ApplicationController.h"
#include "AntInput.h"
#include "DownloadManager.h"

// 判断是否在windows平台
#ifdef Q_OS_WIN

#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#pragma comment(lib, "Dwmapi.lib")

#endif  // Q_OS_WIN

// 检测鼠标是否在边缘
enum ResizeRegion {
	None, Left, Right, Top, Bottom,
	TopLeft, TopRight, BottomLeft, BottomRight
};

class VideoDownloader : public QWidget
{
	Q_OBJECT
public:
	VideoDownloader(QWidget* parent = nullptr);
	~VideoDownloader();
protected:
	void resizeEvent(QResizeEvent* event) override;
	bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
	void showEvent(QShowEvent* event) override;
	void moveEvent(QMoveEvent* event) override;
	void changeEvent(QEvent* event) override;

	#ifdef Q_OS_LINUX
	bool eventFilter(QObject* obj, QEvent* event);
	void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	#endif

signals:
	void resized(int parentW, int parentH);
	void playMaximizeAnim();
	void showStandardDialog(QString title, QString text);
	void windowMoved(QPoint globalPos); // 窗口移动时发出信号

	#ifdef Q_OS_LINUX
private:
	void updateCursor(const QPoint& pos);
private:
	int edgeWidth = 10;
	QSize originalSize;
	QRect normalGeometry;
	Qt::Edges currentEdge; // 鼠标当前所在边/角
	#endif

private:
	Ui::QtAntDesignClass ui;
	#ifdef Q_OS_WIN
	HWND m_hwnd;
	#endif
	QPoint dragPos;
	int m_titleBarHeight = 60;		// 标题栏高度
	int m_naviWidth = 62;			// 导航栏宽度
	int m_widgetTotalWidth = 0;		// 标题栏上控件总宽度
	bool	m_disableShadow = false;
	bool	m_beforeMax = false;	// 最小化之前是最大化的窗口状态

	// 窗口缩放
	ResizeRegion m_resizeRegion = None;
	QPoint m_dragStartPos;
	QRect m_initialGeometry;
	bool m_isLockCursor = false;	// 锁定光标样式

	// 按钮数据结构
	struct ButtonInfo
	{
		CustomToolButton* button;
		QString normalIcon;
		QString activeIcon;
		QWidget* page;
	};
	QList<ButtonInfo> buttonInfos;

	// 窗口动画
	QPropertyAnimation* maxScaleAnim = nullptr;
	QPropertyAnimation* miniScaleAnim = nullptr;
	QRect m_windowStartRect;			// 动画开始前的位置

	// 堆叠窗口
	SlideStackedWidget* stackedWidget = nullptr;

	// 最小化 最大化 关闭 按钮
	QToolButton* btnMin = nullptr;
	QToolButton* btnMax = nullptr;
	QToolButton* btnClose = nullptr;

	QRect m_minRect;
	QRect m_maxRect;
	QRect m_closeRect;

	int titleBarSpacing = 6;
	int totalSpacingWidth = 0;
	int rightMargin = 12;

	// 物理像素
	int m_widgetTotalWidthPhysicalPixels = 0;
	int m_titleLeftTotalWidthPhysicalPixels = 0;
	int m_titleBarHeightPhysicalPixels = 0;

	AntInput* antInput = nullptr;

	//downloadmanager
	QSharedPointer<ApplicationController> appController;
	QSharedPointer<DownloadManager> downloadManager;
};
