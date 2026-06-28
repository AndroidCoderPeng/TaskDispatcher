#ifndef WEBSOCKETOBSERVER_HPP
#define WEBSOCKETOBSERVER_HPP

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>

#include "GlobalDefinition.hpp"

class WebSocketObserver : public QObject {
  Q_OBJECT
public:
  explicit WebSocketObserver(QObject *parent = nullptr);
  ~WebSocketObserver() override;

  static inline WebSocketObserver *get() {
    static WebSocketObserver instance;
    return &instance;
  }

  void startServer(QString ip, quint16 port = 12345);

  void stopServer();

  bool isServerRunning() const;

  void sendMessage(const QString &message);

signals:
  void signalNoClient();

  void signalServerStateChanged(const WebSocketState &state);

  void signalDataReceived(const QString &message);

private slots:
  void onNewConnection();

  void onMessageReceived(const QString &message);

  void onClientDisconnected();

private:
  QWebSocketServer *server = nullptr;
  QList<QWebSocket *> clients;
};

#endif // WEBSOCKETOBSERVER_HPP
