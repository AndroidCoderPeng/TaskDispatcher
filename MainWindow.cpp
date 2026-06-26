#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "Logger.hpp"
#include "WebSocketObserver.hpp"

#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // 自动更新日期和星期、显示当前时间
  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this]() {
    ui->dateLabel->setText(
        QDate::currentDate().toString("yyyy年MM月dd | dddd"));
    ui->timeLabel->setText(QDateTime::currentDateTime().toString("HH:mm:ss"));
  });
  timer->start(1000);

  // 清除QComboBox的QAbstractItemView::item默认QSS
  ui->ipv4Box->setView(new QListView());

  // 连接顶部菜单信号和槽
  connect(ui->actionImportData, &QAction::triggered, this,
          &MainWindow::onActionImportDataClicked);
  connect(ui->actionExportData, &QAction::triggered, this,
          &MainWindow::onActionExportDataClicked);
  connect(ui->actionExit, &QAction::triggered, this,
          &MainWindow::onActionCloseClicked);

  connect(ui->actionEmailSetting, &QAction::triggered, this,
          &MainWindow::onActionEmailSettingClicked);
  connect(ui->actionWeWorkSetting, &QAction::triggered, this,
          &MainWindow::onActionWeWorkSettingClicked);
  connect(ui->actionTaskOvertimeSetting, &QAction::triggered, this,
          &MainWindow::onActionOvertimeSettingClicked);
  connect(ui->actionResetTaskSetting, &QAction::triggered, this,
          &MainWindow::onActionResetTaskSettingClicked);
  connect(ui->actionRandomTimeSetting, &QAction::triggered, this,
          &MainWindow::onActionRandomTimeSettingClicked);
  connect(ui->actionSkipHolidaySetting, &QAction::toggled, this,
          &MainWindow::onActionSkipHolidaySettingToggled);
  connect(ui->actionOpenResetTaskSetting, &QAction::toggled, this,
          &MainWindow::onActionOpenResetTaskSettingToggled);
  connect(ui->actionOpenRandomTimeSetting, &QAction::toggled, this,
          &MainWindow::onActionOpenRandomTimeSettingToggled);

  connect(ui->actionDarkTheme, &QAction::toggled, this,
          &MainWindow::onActionDarkThemeToggled);

  connect(ui->actionSyncData, &QAction::triggered, this,
          &MainWindow::onActionSyncDataClicked);

  connect(ui->actionTestEmail, &QAction::triggered, this,
          &MainWindow::onActionTestEmailClicked);
  connect(ui->actionTextWx, &QAction::triggered, this,
          &MainWindow::onActionTextWxClicked);
  connect(ui->actionQuestion, &QAction::triggered, this,
          &MainWindow::onActionQuestionClicked);
  connect(ui->actionProjectSite, &QAction::triggered, this,
          &MainWindow::onActionProjectSiteTriggered);
  connect(ui->actionAbout, &QAction::triggered, this,
          &MainWindow::onActionAboutTriggered);

  // 连接按钮信号和槽
  connect(ui->addTaskButton, &QPushButton::clicked, this,
          &MainWindow::onAddTaskButtonClicked);
  connect(ui->openSocketButton, &QPushButton::clicked, this,
          &MainWindow::onOpenSocketButtonClicked);
}

void MainWindow::onActionImportDataClicked() {
  Logger::Tag("MainWindow").d("Import data action clicked");
}

void MainWindow::onActionExportDataClicked() {
  Logger::Tag("MainWindow").d("Import data action clicked");
}

void MainWindow::onActionCloseClicked() {
  if (QMessageBox::question(this, "确认", "确定要退出程序吗？",
                            QMessageBox::Yes | QMessageBox::No) ==
      QMessageBox::Yes) {
    QTimer::singleShot(0, this, &QCoreApplication::quit);
  }
}

void MainWindow::onActionEmailSettingClicked() {
  Logger::Tag("MainWindow").d("Email setting action clicked");
}

void MainWindow::onActionWeWorkSettingClicked() {
  Logger::Tag("MainWindow").d("WeWork setting action clicked");
}

void MainWindow::onActionOvertimeSettingClicked() {
  Logger::Tag("MainWindow").d("Overtime setting action clicked");
}

void MainWindow::onActionResetTaskSettingClicked() {
  Logger::Tag("MainWindow").d("Reset task setting action clicked");
}

void MainWindow::onActionRandomTimeSettingClicked() {
  Logger::Tag("MainWindow").d("Random time setting action clicked");
}

void MainWindow::onActionSkipHolidaySettingToggled(bool checked) {
  Logger::Tag("MainWindow")
      .dFmt("Skip holiday setting toggled: %s", checked ? "true" : "false");
}

void MainWindow::onActionOpenResetTaskSettingToggled(bool checked) {
  Logger::Tag("MainWindow")
      .dFmt("Open reset task setting toggled: %s", checked ? "true" : "false");
}

void MainWindow::onActionOpenRandomTimeSettingToggled(bool checked) {
  Logger::Tag("MainWindow")
      .dFmt("Open random time setting toggled: %s", checked ? "true" : "false");
}

void MainWindow::onActionDarkThemeToggled(bool checked) {
  if (checked) {
    QFile styleFile(":/style_dark.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
      qApp->setStyleSheet(styleFile.readAll());
      styleFile.close();
    }
  } else {
    QFile styleFile(":/style_light.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
      qApp->setStyleSheet(styleFile.readAll());
      styleFile.close();
    }
  }
}

void MainWindow::onActionSyncDataClicked() {
  Logger::Tag("MainWindow").d("Sync data action clicked");
}

void MainWindow::onActionTestEmailClicked() {
  Logger::Tag("MainWindow").d("Test email action clicked");
}

void MainWindow::onActionTextWxClicked() {
  Logger::Tag("MainWindow").d("Test WeWork action clicked");
}

void MainWindow::onActionQuestionClicked() {
  Logger::Tag("MainWindow").d("Question action clicked");
}

void MainWindow::onActionProjectSiteTriggered() {
  QDesktopServices::openUrl(
      QUrl("https://github.com/AndroidCoderPeng/TaskDispatcher"));
}

void MainWindow::onActionAboutTriggered() {
  QMessageBox::about(this, "关于任务调度器",
                     "<h2>任务调度器 v1.0.0</h2>"
                     "<p>基于 Qt 5 的跨平台任务调度工具</p>"
                     "<hr>"
                     "<p><b>作者：</b>AndroidCoderPeng</p>"
                     "<p><b>邮箱：</b><a "
                     "href='mailto:AndroidCoderPeng'>290677893@qq.com</a></p>"
                     "<hr>"
                     "<p>支持 Windows / Linux / Mac 平台</p>");
}

void MainWindow::bindIpAddresses(const QList<QString> &ips) {
  ui->ipv4Box->clear();
  for (const QString &ip : ips) {
    ui->ipv4Box->addItem(ip);
  }
}

void MainWindow::onAddTaskButtonClicked() {
  Logger::Tag("MainWindow").d("Add task button clicked");
}

void MainWindow::onOpenSocketButtonClicked() {
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
