#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "Logger.hpp"
#include "WebSocketObserver.hpp"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // 清除QComboBox的QAbstractItemView::item默认QSS
  ui->ipv4Box->setView(new QListView());

  connect(ui->openSocketButton, &QPushButton::clicked, this,
          &MainWindow::onOpenSocketButton);
}

void MainWindow::bindIpAddresses(const QList<QString> &ips) {
  ui->ipv4Box->clear();
  for (const QString &ip : ips) {
    ui->ipv4Box->addItem(ip);
  }
}

void MainWindow::onOpenSocketButton() {
  if (WebSocketObserver::get()->isServerRunning()) {
    if (QMessageBox::question(this, "确认", "确定要关闭通信服务吗？",
                              QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
      WebSocketObserver::get()->stopServer();
    }
  } else {
    WebSocketObserver::get()->startServer(ui->ipv4Box->currentText());
  }
}

void MainWindow::slotServerStateChanged(const WebSocketState &state) {
  if (state == WebSocketState::RUNNING) {
    ui->socketIconView->setPixmap(QPixmap(":/socket_listening.png"));
    ui->socketStateView->setText("通信服务已开启");
    ui->openSocketButton->setText("关闭通信服务");
  } else {
    ui->socketIconView->setPixmap(QPixmap(":/socket_shutdown.png"));
    ui->socketStateView->setText("通信服务已关闭");
    ui->openSocketButton->setText("开启通信服务");
  }
}

void MainWindow::slotDataReceived(const QString &message) {
  Logger::Tag("MainWindow")
      .dFmt("Received message: %s", message.toStdString().c_str());
}

MainWindow::~MainWindow() { delete ui; }
