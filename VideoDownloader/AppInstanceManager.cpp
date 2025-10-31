#include "AppInstanceManager.h"

AppInstanceManager::AppInstanceManager(QObject* parent)
	: QObject(parent)
{
	m_localServer = new QLocalServer(this);
	connect(m_localServer, &QLocalServer::newConnection, this, &AppInstanceManager::handleNewConnection);
}

bool AppInstanceManager::isAnotherInstanceRunning(const QString& serverName)
{
	QLocalSocket socket;
	socket.connectToServer(serverName);
	if (socket.waitForConnected(1000)) return true;

	QLocalServer::removeServer(serverName);
	if (!m_localServer->listen(serverName))
	{
		QMessageBox::critical(nullptr, "Error", "Failed to start local server:" + m_localServer->errorString());
		return false;
	}
	return false;
}

void AppInstanceManager::sendToServer(const QString& serverName, const QString& message)
{
	QLocalSocket socket;
	socket.connectToServer(serverName);
	if (socket.waitForConnected(3000))
	{
		socket.write(message.toUtf8());
		socket.waitForBytesWritten();
		socket.disconnectFromServer();
	}
}

void AppInstanceManager::handleNewConnection()
{
	QLocalSocket* socket = m_localServer->nextPendingConnection();
	connect(socket, &QLocalSocket::readyRead, [this, socket]() {
		QString message = QString::fromUtf8(socket->readAll());
		emit messageReceived(message);
		socket->deleteLater();
		});
}