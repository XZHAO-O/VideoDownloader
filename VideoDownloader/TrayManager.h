#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMainWindow>

class TrayManager : public QObject
{
	Q_OBJECT

public:
	explicit TrayManager(QWidget* mainWindow, QObject* parent = nullptr);
	~TrayManager();

	void show();
	void hide();
	void showMessage(const QString& title, const QString& message,
		QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
		int timeout = 1500);

public slots:
	void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void toggleWindowVisibility();
	void showWindow();
	void hideWindow();

signals:
	void quitRequested();

private:
	void createTrayIcon();
	void createTrayMenu();

	QWidget* m_mainWindow;
	QSystemTrayIcon* m_trayIcon;
	QMenu* m_trayMenu;

	QAction* m_showAction;
	QAction* m_hideAction;
	QAction* m_quitAction;
};