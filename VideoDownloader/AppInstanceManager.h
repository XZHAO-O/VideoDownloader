#pragma once

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>

class AppInstanceManager : public QObject
{
	Q_OBJECT
public:
	explicit AppInstanceManager(QObject* parent = nullptr);

	// 检查是否已有实例运行，如果没有则创建服务器
	bool isAnotherInstanceRunning(const QString& serverName);

	// 向已运行的实例发送消息
	void sendToServer(const QString& serverName, const QString& message);

	// 待添加函数，启动新程序后像就程序发送消息使其将应用提升到最上层

signals:
	void messageReceived(const QString& message);

private slots:
	void handleNewConnection();

private:
	QLocalServer* m_localServer;
};