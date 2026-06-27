#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "ConfigStore.hpp"
#include "EmailSettingDialog.hpp"
#include "Logger.hpp"
#include "ResetTaskSettingDialog.hpp"
#include "WebSocketObserver.hpp"
#include "WxSettingDialog.hpp"

#include <QDesktopServices>
#include <QDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimeEdit>
#include <QTimer>
#include <QVBoxLayout>

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

  connect(ui->actionCaptureScreen, &QAction::triggered, this,
          &MainWindow::onActionCaptureScreenClicked);
  connect(ui->actionOpenTargetApp, &QAction::triggered, this,
          &MainWindow::onActionOpenTargetAppClicked);

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
  // if (history.isEmpty()) {
  //   QMessageBox::warning(this, "警告", "没有数据可以保存");
  //   return;
  // }
  // const QString filePath =
  //     QFileDialog::getSaveFileName(this, "保存日志", "", "文本文件 (*.txt)");
  // if (filePath.isEmpty()) {
  //   QMessageBox::warning(this, "警告", "未选择保存文件");
  //   return;
  // }
  // QFile file(filePath);
  // if (!file.open(QIODevice::WriteOnly | QIODevice::Text |
  //                QIODevice::Truncate)) {
  //   QMessageBox::critical(this, "错误", "无法打开文件：" +
  //   file.errorString()); return;
  // }
  // QTextStream out(&file);
  // const QList<PortMessage> &listRef = history;
  // for (const auto &msg : listRef) {
  //   const QString hexData = Utils::formatByteArray(msg.data);
  //   const auto line = QString("[%1]【%2】%3\n")
  //                         .arg(msg.formattedTime, msg.direction, hexData);
  //   out << line;
  // }
  // file.close();
}

void MainWindow::onActionCloseClicked() {
  if (QMessageBox::question(this, "确认", "确定要退出程序吗？",
                            QMessageBox::Yes | QMessageBox::No) ==
      QMessageBox::Yes) {
    QTimer::singleShot(0, this, &QCoreApplication::quit);
  }
}

void MainWindow::onActionEmailSettingClicked() {
  EmailSettingDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    const auto result = dialog.getInputValue();
    if (result.first) {
      const EmailConfig &cfg = result.second;

      QJsonObject obj;
      obj["emailTitle"] = cfg.emailTitle;
      obj["senderEmail"] = cfg.senderEmail;
      obj["authCode"] = cfg.authCode;
      obj["receiverEmail"] = cfg.receiverEmail;
      ConfigStore::get().save("emailConfig", obj);
      QMessageBox::information(this, "提示", "邮箱配置已保存");
    }
  }
}

void MainWindow::onActionWeWorkSettingClicked() {
  WxSettingDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    const auto result = dialog.getInputValue();
    if (result.first) {
      const WxConfig &cfg = result.second;

      QJsonObject obj;
      obj["messageTitle"] = cfg.messageTitle;
      obj["wxKey"] = cfg.wxKey;
      ConfigStore::get().save("wxConfig", obj);
      QMessageBox::information(this, "提示", "企业微信配置已保存");
    }
  }
}

void MainWindow::onActionOvertimeSettingClicked() {
  int defaultValue = 30; // 默认 30 秒
  QJsonObject saved = ConfigStore::get().load("overtimeConfig");
  if (saved.contains("seconds")) {
    defaultValue = saved["seconds"].toInt();
  }

  bool ok = false;
  const int seconds =
      QInputDialog::getInt(this, "任务超时时间", "请输入任务超时时间（秒）",
                           defaultValue, 10, 120, 1, &ok);

  if (ok) {
    QJsonObject obj;
    obj["seconds"] = seconds;
    ConfigStore::get().save("overtimeConfig", obj);
    QMessageBox::information(
        this, "提示", QString("任务超时时间已设置为 %1 秒").arg(seconds));
  }
}

void MainWindow::onActionResetTaskSettingClicked() {
  ResetTaskSettingDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    const auto result = dialog.getInputValue();
    if (result.first) {
      const ResetTaskConfig &cfg = result.second;

      QJsonObject obj;
      obj["time"] = cfg.time;
      ConfigStore::get().save("resetTaskConfig", obj);
      QMessageBox::information(
          this, "提示", QString("重置任务时间已设置为 %1").arg(cfg.time));
    }
  }
}

void MainWindow::onActionRandomTimeSettingClicked() {
  int defaultValue = 5; // 默认 5 分钟
  QJsonObject saved = ConfigStore::get().load("randomTimeConfig");
  if (saved.contains("minutes")) {
    defaultValue = saved["minutes"].toInt();
  }

  bool ok = false;
  const int minutes =
      QInputDialog::getInt(this, "任务波动时间", "请输入任务波动时间（分钟）",
                           defaultValue, 3, 30, 1, &ok);

  if (ok) {
    QJsonObject obj;
    obj["minutes"] = minutes;
    ConfigStore::get().save("randomTimeConfig", obj);
    QMessageBox::information(
        this, "提示", QString("任务波动时间已设置为 %1 分钟").arg(minutes));
  }
}

void MainWindow::onActionSkipHolidaySettingToggled(bool checked) {
  QJsonObject obj;
  obj["skipHoliday"] = checked;
  ConfigStore::get().save("skipHolidayConfig", obj);
}

void MainWindow::onActionOpenResetTaskSettingToggled(bool checked) {
  QJsonObject obj;
  obj["openResetTask"] = checked;
  ConfigStore::get().save("openResetTaskConfig", obj);
}

void MainWindow::onActionOpenRandomTimeSettingToggled(bool checked) {
  QJsonObject obj;
  obj["openRandomTime"] = checked;
  ConfigStore::get().save("openRandomTimeConfig", obj);
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

void MainWindow::onActionCaptureScreenClicked() {
  Logger::Tag("MainWindow").d("Capture screen action clicked");
}

void MainWindow::onActionOpenTargetAppClicked() {
  Logger::Tag("MainWindow").d("Open target app action clicked");
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
  QMessageBox::about(this, "关于",
                     "<h2>任务调度器 v1.0.0</h2>"
                     "<p>基于 Qt 5 的跨平台工具</p>"
                     "<hr>"
                     "<p><b>作者：</b>AndroidCoderPeng</p>"
                     "<p><b>邮箱：</b><a "
                     "href='mailto:AndroidCoderPeng'>290677893@qq.com</a></p>"
                     "<hr>"
                     "<p>支持 Windows / Linux / Mac "
                     "平台。Mac平台需要自行下载Qt编译链编译</p>");
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
