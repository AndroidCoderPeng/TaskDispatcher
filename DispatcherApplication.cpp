#include "DispatcherApplication.hpp"

#include "ConfigStore.hpp"
#include "Logger.hpp"

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
}

void DispatcherApplication::initMainWindow() {
  mainWindowPtr = new MainWindow();
  mainWindowPtr->setWindowTitle("任务调度器");
  mainWindowPtr->setWindowIcon(QIcon(":/application.png"));

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

  // 在 QApplication 销毁前主动将配置写入磁盘，避免静态单例析构时
  ConfigStore::get().flush();

  Logger::Tag("DispatcherApplication")
      .i("DispatcherApplication is being destroyed, cleaning up resources.");
}