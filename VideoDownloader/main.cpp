#include <QApplication>
#include <QFontDatabase>
#include <QDir>

#include "AppInstanceManager.h"
#include "TrayManager.h"
#include "VideoDownloader.h"
#include "Instrumentor.h"

int main(int argc, char* argv[])
{
	const QString serverName = "VideoDownloader_XZHAO_O";
	AppInstanceManager appInstanceManager;
	if (appInstanceManager.isAnotherInstanceRunning(serverName))
		return 0;

	#ifdef Q_OS_LINUX
	qputenv("QT_QPA_PLATFORM", "xcb"); // 避免 Wayland
	#endif
	QApplication app(argc, argv);

	// 获取当前可执行文件所在的路径（通常是 Debug 或 Release 目录）
	QString currentDir = QCoreApplication::applicationDirPath();
	// 跳上两级，达到项目根目录
	QDir dir(currentDir);
	dir.cdUp();
	dir.cdUp();
	// 设置当前工作目录为项目根目录
	QDir::setCurrent(dir.absolutePath());

	QApplication::setApplicationName("VideoDownloader");
	QApplication::setApplicationDisplayName("VideoDownloader");
	QApplication::setQuitOnLastWindowClosed(false);

	// 加载google字体
	//int fontId = QFontDatabase::addApplicationFont(":/fonts/NotoSansSC-Regular.ttf");
	//if (fontId != -1)
	//{
	//	QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
	//	QFont font(family);
	//	font.setHintingPreference(QFont::PreferNoHinting);
	//	QApplication::setFont(font);
	//}
	//else
	//{
	//	qWarning("字体加载失败！");
	//}

	BENCHMARKING_START();
	BENCHMARKING_FUNCTION();
	//将appcontroller提出来再传入
	VideoDownloader window;
	// 创建托盘管理器
	TrayManager trayManager(&window);

	// 连接退出信号
	QObject::connect(&trayManager, &TrayManager::quitRequested, &app, &QApplication::quit);

	// 显示托盘图标
	trayManager.show();

	// 可选：显示启动消息
	trayManager.showMessage("应用程序已启动",
		"程序已在系统托盘中运行\n双击图标显示/隐藏窗口",
		QSystemTrayIcon::Information);
	window.show();

	int ret = app.exec();
	BENCHMARKING_STOP();
	return ret;
}