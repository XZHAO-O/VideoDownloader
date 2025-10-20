#include <QApplication>
#include <QFontDatabase>
#include <QDir>

#include "VideoDownloader.h"

int main(int argc, char* argv[])
{
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

	// 加载google字体
	int fontId = QFontDatabase::addApplicationFont(":/fonts/NotoSansSC-Regular.ttf");
	if (fontId != -1)
	{
		QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
		QFont font(family);
		font.setHintingPreference(QFont::PreferNoHinting);
		QApplication::setFont(font);
	}
	else
	{
		qWarning("字体加载失败！");
	}

	VideoDownloader window;
	window.show();

	return app.exec();
}