#include "WebSocketObserver.hpp"

#include "Logger.hpp"

WebSocketObserver::WebSocketObserver(QObject *parent) : QObject(parent) {}

void WebSocketObserver::startServer(QString ip, quint16 port) {
  if (server) {
    Logger::Tag("WebSocketObserver").w("Server already running.");
    return;
  }

  server = new QWebSocketServer("TaskDispatcher",
                                QWebSocketServer::NonSecureMode, this);

  if (!server->listen(QHostAddress(ip), port)) {
    Logger::Tag("WebSocketObserver")
        .eFmt("Failed to start WebSocket server on port %d: %s", port,
              server->errorString().toUtf8().constData());
    delete server;
    server = nullptr;
    return;
  }

  connect(server, &QWebSocketServer::newConnection, this,
          &WebSocketObserver::onNewConnection);

  Logger::Tag("WebSocketObserver")
      .dFmt("WebSocket server started on %s:%d", ip.toUtf8().constData(), port);
  emit signalServerStateChanged(WebSocketState::RUNNING);
}

void WebSocketObserver::stopServer() {
  if (!server)
    return;

  server->close();

  for (QWebSocket *client : clients) {
    client->close();
    client->deleteLater();
  }
  clients.clear();

  server->deleteLater();
  server = nullptr;

  Logger::Tag("WebSocketObserver").i("WebSocket server stopped.");
  emit signalServerStateChanged(WebSocketState::SHUTDOWN);
}

bool WebSocketObserver::isServerRunning() const {
  return server != nullptr && server->isListening();
}

void WebSocketObserver::onNewConnection() {
  while (server->hasPendingConnections()) {
    QWebSocket *client = server->nextPendingConnection();
    clients.append(client);

    connect(client, &QWebSocket::textMessageReceived, this,
            &WebSocketObserver::onMessageReceived);
    connect(client, &QWebSocket::disconnected, this,
            &WebSocketObserver::onClientDisconnected);

    const QString clientInfo = QString("%1:%2")
                                   .arg(client->peerAddress().toString())
                                   .arg(client->peerPort());
    Logger::Tag("WebSocketObserver")
        .dFmt("Client connected: %s", clientInfo.toUtf8().constData());
  }
}

void WebSocketObserver::onMessageReceived(const QString &message) {
  emit signalDataReceived(message);
}

void WebSocketObserver::onClientDisconnected() {
  QWebSocket *client = qobject_cast<QWebSocket *>(sender());
  if (client) {
    const QString clientInfo = QString("%1:%2")
                                   .arg(client->peerAddress().toString())
                                   .arg(client->peerPort());
    Logger::Tag("WebSocketObserver")
        .dFmt("Client disconnected: %s", clientInfo.toUtf8().constData());
    clients.removeAll(client);
    client->deleteLater();
  }
}

WebSocketObserver::~WebSocketObserver() { stopServer(); }