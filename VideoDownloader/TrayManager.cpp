#include "TrayManager.h"

#include <QApplication>
#include <QStyle>

TrayManager::TrayManager(QWidget* mainWindow, QObject* parent)
	: QObject(parent)
	, m_mainWindow(mainWindow)
	, m_trayIcon(nullptr)
	, m_trayMenu(nullptr)
{
	createTrayIcon();
	createTrayMenu();
}

TrayManager::~TrayManager()
{
	if (m_trayIcon) {
		m_trayIcon->hide();
		delete m_trayIcon;
	}
}

void TrayManager::createTrayIcon()
{
	m_trayIcon = new QSystemTrayIcon(this);

	// 使用应用程序图标或默认图标
	if (!QApplication::windowIcon().isNull()) {
		m_trayIcon->setIcon(QApplication::windowIcon());
	}
	else {
		m_trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
	}

	m_trayIcon->setToolTip(QApplication::applicationName());

	connect(m_trayIcon, &QSystemTrayIcon::activated,
		this, &TrayManager::onTrayIconActivated);
}

void TrayManager::createTrayMenu()
{
	m_trayMenu = new QMenu();

	m_showAction = new QAction("显示窗口", this);
	m_hideAction = new QAction("隐藏窗口", this);
	m_quitAction = new QAction("退出", this);

	connect(m_showAction, &QAction::triggered, this, &TrayManager::showWindow);
	connect(m_hideAction, &QAction::triggered, this, &TrayManager::hideWindow);
	connect(m_quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

	m_trayMenu->addAction(m_showAction);
	m_trayMenu->addAction(m_hideAction);
	m_trayMenu->addSeparator();
	m_trayMenu->addAction(m_quitAction);

	m_trayIcon->setContextMenu(m_trayMenu);
}

void TrayManager::show()
{
	if (m_trayIcon) {
		m_trayIcon->show();
	}
}

void TrayManager::hide()
{
	if (m_trayIcon) {
		m_trayIcon->hide();
	}
}

void TrayManager::showMessage(const QString& title, const QString& message,
	QSystemTrayIcon::MessageIcon icon, int timeout)
{
	if (m_trayIcon) {
		m_trayIcon->showMessage(title, message, icon, timeout);
	}
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
	case QSystemTrayIcon::DoubleClick:
		//case QSystemTrayIcon::Trigger:
		toggleWindowVisibility();
		break;
	default:
		break;
	}
}

void TrayManager::toggleWindowVisibility()
{
	if (m_mainWindow->isVisible()) {
		hideWindow();
	}
	else {
		showWindow();
	}
}

void TrayManager::showWindow()
{
	m_mainWindow->show();
	m_mainWindow->activateWindow();
	m_mainWindow->raise();
}

void TrayManager::hideWindow()
{
	m_mainWindow->hide();
}