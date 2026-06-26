#include "DispatcherApplication.hpp"

#include "Logger.hpp"
#include "WebSocketObserver.hpp"

#include <QFile>
#include <QFontDatabase>
#include <QHostAddress>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QScreen>
#include <QTimer>

DispatcherApplication::DispatcherApplication(int &argc, char **argv)
    : QApplication(argc, argv), mainWindowPtr(nullptr) {

  int fontId = QFontDatabase::addApplicationFont(":/msyh.ttc");
  if (fontId != -1) {
    QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
    QFont font(fontFamily);
    font.setPointSize(10);
    setFont(font);
  } else {
    Logger::Tag("DispatcherApplication").w("Failed to load font.");
  }

  QFile styleFile(":/style_light.qss");
  if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
    QString style = QString::fromUtf8(styleFile.readAll());
    setStyleSheet(style);
    styleFile.close();
  } else {
    Logger::Tag("DispatcherApplication").w("Failed to load style.");
  }

  initMainWindow();

  const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
  QList<QString> ips;
  for (const QHostAddress &address : addresses) {
    if (address.protocol() == QAbstractSocket::IPv4Protocol &&
        address != QHostAddress::LocalHost && !address.isLoopback()) {
      ips.append(address.toString().toUtf8().constData());
    }
  }

  if (ips.isEmpty()) {
    QMessageBox::warning(nullptr, "警告",
                         "未找到有效的IPv4地址，应用程序将退出。");
    QTimer::singleShot(0, this, &QCoreApplication::quit);
    return;
  }

  mainWindowPtr->bindIpAddresses(ips);

  // 启动 WebSocket 服务端并初始化WebSocket信号连接
  QObject::connect(WebSocketObserver::get(),
                   &WebSocketObserver::signalServerStateChanged, mainWindowPtr,
                   &MainWindow::slotServerStateChanged);
  QObject::connect(WebSocketObserver::get(),
                   &WebSocketObserver::signalDataReceived, mainWindowPtr,
                   &MainWindow::slotDataReceived);
}

void DispatcherApplication::initMainWindow() {
  mainWindowPtr = new MainWindow();
  mainWindowPtr->setWindowTitle("任务调度器");
  // mainWindowPtr->setWindowIcon(QIcon(":/application.png"));

  const QRect rect = primaryScreen()->availableGeometry();
  mainWindowPtr->move((rect.width() - mainWindowPtr->width()) / 2,
                      (rect.height() - mainWindowPtr->height()) / 2);
  mainWindowPtr->show();
  Logger::Tag("DispatcherApplication").i("MainWindow initialized and shown.");
}

DispatcherApplication::~DispatcherApplication() {
  // QThread 子对象会随 Application 销毁自动 quit/wait
  delete mainWindowPtr;
  mainWindowPtr = nullptr;
  Logger::Tag("DispatcherApplication")
      .i("DispatcherApplication is being destroyed, cleaning up resources.");
}