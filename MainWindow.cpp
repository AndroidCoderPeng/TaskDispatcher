#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AddTaskDialog.hpp"
#include "ChinaHolidayManager.hpp"
#include "ConfigStore.hpp"
#include "EmailSettingDialog.hpp"
#include "Logger.hpp"
#include "MailSender.hpp"
#include "ResetTaskSettingDialog.hpp"
#include "TaskItemWidget.hpp"
#include "TaskStore.hpp"
#include "ToastWidget.hpp"
#include "WebSocketObserver.hpp"
#include "WsProtocol.hpp"
#include "WxMessageSender.hpp"
#include "WxSettingDialog.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
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
    updateCountDown();
  });
  timer->start(1000);

  const int taskCount = TaskStore::get().loadAll().size();
  Logger::Tag("MainWindow").dFmt("Loaded %d tasks from database", taskCount);
  ui->taskCountLabel->setText(QString::number(taskCount));
  updateTaskListWidget();

  // 从 ConfigStore 恢复通知方式选中状态
  {
    const QJsonObject saved = ConfigStore::get().load("notifyMethodConfig");
    const QString method =
        saved.contains("method") ? saved["method"].toString() : QString();
    if (method == "wx") {
      ui->wxRadioButton->setChecked(true);
    } else {
      // 默认选中邮箱通知
      ui->emailRadioButton->setChecked(true);
    }
  }

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
  connect(ui->actionTaskDelayTimeSetting, &QAction::triggered, this,
          &MainWindow::onActionDelayTimeSettingClicked);
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
  connect(ui->actionKillTargetApp, &QAction::triggered, this,
          &MainWindow::onActionKillTargetAppClicked);

  connect(ui->actionTestEmail, &QAction::triggered, this,
          &MainWindow::onActionTestEmailClicked);
  connect(ui->actionTextWx, &QAction::triggered, this,
          &MainWindow::onActionTextWxClicked);
  connect(ui->actionProjectSite, &QAction::triggered, this,
          &MainWindow::onActionProjectSiteTriggered);
  connect(ui->actionAbout, &QAction::triggered, this,
          &MainWindow::onActionAboutTriggered);

  // 连接按钮信号和槽
  connect(ui->executeTaskButton, &QPushButton::clicked, this,
          &MainWindow::onExecuteTaskButtonClicked);
  connect(ui->addTaskButton, &QPushButton::clicked, this,
          &MainWindow::onAddTaskButtonClicked);
  connect(ui->openSocketButton, &QPushButton::clicked, this,
          &MainWindow::onOpenSocketButtonClicked);

  // 连接 RadioButton 信号：通知方式切换
  connect(ui->emailRadioButton, &QRadioButton::toggled, this,
          &MainWindow::onNotifyMethodChanged);
  connect(ui->wxRadioButton, &QRadioButton::toggled, this,
          &MainWindow::onNotifyMethodChanged);

  // 连接节假日数据同步信号
  const auto holidayManager = ChinaHolidayManager::get();
  connect(
      holidayManager, &ChinaHolidayManager::signalSyncSuccess, this,
      [this](const QString &message) { ToastWidget::showInfo(this, message); });
  connect(holidayManager, &ChinaHolidayManager::signalSyncError, this,
          [this](const QString &message) {
            ToastWidget::showError(this, message);
          });
}

void MainWindow::onActionImportDataClicked() {
  const QString filePath =
      QFileDialog::getOpenFileName(this, "导入数据", "", "配置文件 (*.json)");
  if (filePath.isEmpty()) {
    Logger::Tag("MainWindow").i("Import data canceled");
    return;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    ToastWidget::showError(this, "无法打开文件：" + file.errorString());
    return;
  }

  const QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    ToastWidget::showError(
        this, QString("JSON 解析失败：%1").arg(parseError.errorString()));
    return;
  }

  if (!doc.isObject()) {
    ToastWidget::showError(this, "文件格式不正确，根节点必须是 JSON 对象");
    return;
  }

  const QJsonObject root = doc.object();

  // 导入配置
  int configCount = 0;
  if (root.contains("configs") && root["configs"].isObject()) {
    const QJsonObject configs = root["configs"].toObject();
    for (auto it = configs.begin(); it != configs.end(); ++it) {
      if (it.value().isObject()) {
        ConfigStore::get().save(it.key(), it.value().toObject());
        ++configCount;
      }
    }
  }

  // 导入任务
  int taskCount = 0;
  if (root.contains("tasks") && root["tasks"].isArray()) {
    const QJsonArray tasks = root["tasks"].toArray();
    for (const QJsonValue &val : tasks) {
      if (!val.isObject())
        continue;
      const QJsonObject obj = val.toObject();
      Task task;
      task.scheduledTime =
          QDateTime::fromString(obj["scheduledTime"].toString(), "HH:mm:ss");
      if (!task.scheduledTime.isValid())
        continue;
      if (TaskStore::get().add(task) > 0) {
        ++taskCount;
      }
    }
  }

  // 刷新界面
  const int totalTasks = TaskStore::get().loadAll().size();
  ui->taskCountLabel->setText(QString::number(totalTasks));
  updateTaskListWidget();

  ToastWidget::showInfo(
      this,
      QString("成功导入 %1 项配置，%2 条任务").arg(configCount).arg(taskCount));
}

void MainWindow::onActionExportDataClicked() {
  const auto configs = ConfigStore::get().loadAll();
  const auto tasks = TaskStore::get().loadAll();
  if (configs.isEmpty() && tasks.isEmpty()) {
    ToastWidget::showWarning(this, "没有数据可以导出");
    return;
  }

  const QString filePath =
      QFileDialog::getSaveFileName(this, "导出数据", "", "配置文件 (*.json)");
  if (filePath.isEmpty()) {
    Logger::Tag("MainWindow").i("Export data canceled");
    return;
  }
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text |
                 QIODevice::Truncate)) {
    ToastWidget::showError(this, "无法打开文件：" + file.errorString());
    return;
  }

  QJsonObject root;
  root["version"] = 1;
  root["exportTime"] = QDateTime::currentDateTime().toString("HH:mm:ss");

  // 导出配置
  QJsonObject configsObj;
  for (auto it = configs.begin(); it != configs.end(); ++it) {
    configsObj[it.key()] = it.value();
  }
  root["configs"] = configsObj;

  // 导出任务
  QJsonArray tasksArr;
  for (const Task &task : tasks) {
    QJsonObject obj;
    obj["id"] = task.id;
    obj["scheduledTime"] = task.scheduledTime.toString("HH:mm:ss");
    tasksArr.append(obj);
  }
  root["tasks"] = tasksArr;

  const QJsonDocument doc(root);
  file.write(doc.toJson(QJsonDocument::Indented));
  file.close();

  ToastWidget::showInfo(this, QString("成功导出 %1 项配置，%2 条任务")
                                  .arg(configs.size())
                                  .arg(tasks.size()));
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
      ToastWidget::showInfo(this, "邮箱配置已保存");
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
      ToastWidget::showInfo(this, "企业微信配置已保存");
    }
  }
}

// 通过wss发消息给APP唤起目标应用，等delay时间到，通过adb截屏，再通过adb杀掉目标应用
void MainWindow::onActionDelayTimeSettingClicked() {
  int defaultValue = 30; // 默认 30 秒
  QJsonObject saved = ConfigStore::get().load("delayTimeConfig");
  if (saved.contains("seconds")) {
    defaultValue = saved["seconds"].toInt();
  }

  bool ok = false;
  const int seconds =
      QInputDialog::getInt(this, "任务等待时间", "请输入任务等待时间（秒）",
                           defaultValue, 10, 120, 1, &ok);

  if (ok) {
    QJsonObject obj;
    obj["seconds"] = seconds;
    ConfigStore::get().save("delayTimeConfig", obj);
    ToastWidget::showInfo(this,
                          QString("任务等待时间已设置为 %1 秒").arg(seconds));
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

      // 立即刷新倒计时显示
      updateCountDown();

      ToastWidget::showInfo(this,
                            QString("重置任务时间已设置为 %1").arg(cfg.time));
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
    ToastWidget::showInfo(this,
                          QString("任务波动时间已设置为 %1 分钟").arg(minutes));
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
  ChinaHolidayManager::get()->updateChinaHolidayData();
}

void MainWindow::onActionCaptureScreenClicked() {
  // 确保 capture 目录存在
  const QString captureDir =
      QCoreApplication::applicationDirPath() + "/capture";
  QDir dir(captureDir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  // 生成带时间戳的文件名
  const QString fileName =
      QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
  const QString filePath = captureDir + "/" + fileName;

  // 通过 adb 截图并直接导出到本地文件
  QProcess *process = new QProcess(this);
  connect(
      process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
      this, [this, process, filePath](int exitCode, QProcess::ExitStatus) {
        process->deleteLater();

        if (exitCode != 0) {
          const QString err = process->readAllStandardError().trimmed();
          QString tip;
          if (err.contains("no devices")) {
            tip = "未检测到已连接的 Android 设备，请检查 USB 连接或 adb 状态";
          } else if (err.contains("more than one device")) {
            tip = "检测到多个设备连接，请仅保留一台设备";
          } else {
            tip = err;
          }
          MailSender::get()->sendEmail("截屏失败", tip.toStdString().c_str());
          ToastWidget::showError(this, tip);
          return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
          const QString err = "无法写入截图文件: " + filePath;
          MailSender::get()->sendEmail("截屏失败", err);
          ToastWidget::showError(this, err);
          return;
        }
        file.write(process->readAllStandardOutput());
        file.close();

        const QString filePathStr = filePath.toStdString().c_str();
        Logger::Tag("MainWindow")
            .dFmt("Screen capture saved to: %s", filePathStr);
        // TODO 发送邮件或者企业微信通知用户
      });
  process->start("adb", {"exec-out", "screencap", "-p"});
}

void MainWindow::onActionOpenTargetAppClicked() {
  // 发送 websocket 消息给客户端，通知客户端打开目标应用
  const auto observer = WebSocketObserver::get();
  if (!observer->isServerRunning()) {
    ToastWidget::showWarning(this, "通信服务未开启，请先开启通信服务");
    return;
  }
  observer->sendMessage(WsProtocol::Action::OPEN_APP);
}

void MainWindow::onActionKillTargetAppClicked() {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          process, &QProcess::deleteLater);
  process->start("adb", {"shell", "am", "force-stop", targetPackage});
}

void MainWindow::onActionTestEmailClicked() {
  QJsonObject obj = ConfigStore::get().load("emailConfig");
  if (obj.isEmpty()) {
    ToastWidget::showWarning(this, "请先配置邮箱信息");
    return;
  }
  MailSender::get()->sendEmail(
      "测试邮件",
      "这是一封来自 TaskDispatcher 的测试邮件，邮件发送功能配置成功！");
}

void MainWindow::onActionTextWxClicked() {
  QJsonObject obj = ConfigStore::get().load("wxConfig");
  if (obj.isEmpty()) {
    ToastWidget::showWarning(this, "请先配置企业微信信息");
    return;
  }
  WxMessageSender::get()->sendMessageAsync(
      "测试企业微信消息",
      "这是一条来自 TaskDispatcher 的企业微信消息，消息发送功能配置成功！");
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
                     "平台。Mac平台需自行下载Qt编译链编译</p>");
}

void MainWindow::bindIpAddresses(const QList<QString> &ips) {
  ui->ipv4Box->clear();
  for (const QString &ip : ips) {
    ui->ipv4Box->addItem(ip);
  }
}

void MainWindow::onExecuteTaskButtonClicked() {
  if (!WebSocketObserver::get()->isServerRunning()) {
    ToastWidget::showWarning(this, "通信服务未开启，请先开启通信服务");
    return;
  }

  if (TaskStore::get().loadAll().isEmpty()) {
    ToastWidget::showWarning(this, "没有任务可以执行，请先添加任务");
    return;
  }

  // TODO
  // 可以执行链式任务了，还需要考虑任务波动时间配置是否开启，以及节假日是否跳过的配置
}

void MainWindow::onAddTaskButtonClicked() {
  AddTaskDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    const auto result = dialog.getInputValue();
    if (result.first) {
      const qint32 newId = TaskStore::get().add(result.second);
      if (newId > 0) {
        // 刷新列表和任务数量
        const int taskCount = TaskStore::get().loadAll().size();
        ui->taskCountLabel->setText(QString::number(taskCount));
        updateTaskListWidget();
      }
    }
  }
}

void MainWindow::updateTaskListWidget() {
  ui->listWidget->clear();
  const QList<Task> tasks = TaskStore::get().loadAll();
  for (const Task &task : tasks) {
    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    item->setData(Qt::UserRole, task.id);

    TaskItemWidget *taskWidget = new TaskItemWidget(task, ui->listWidget);
    item->setSizeHint(taskWidget->sizeHint());
    ui->listWidget->setItemWidget(item, taskWidget);
  }
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

void MainWindow::onNotifyMethodChanged() {
  const QString method = ui->emailRadioButton->isChecked() ? "email" : "wx";
  QJsonObject obj;
  obj["method"] = method;
  ConfigStore::get().save("notifyMethodConfig", obj);
}

void MainWindow::slotNoClient() {
  ToastWidget::showWarning(this, "没有客户端连接到通信服务");
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
  // 响应客户端的消息，处理后续逻辑
  Logger::Tag("MainWindow")
      .dFmt("Received message: %s", message.toStdString().c_str());
}

void MainWindow::updateCountDown() {
  QJsonObject resetTaskConfig = ConfigStore::get().load("resetTaskConfig");
  // 未设置则默认 0 点
  const QString resetTimeStr = resetTaskConfig.contains("time")
                                   ? resetTaskConfig["time"].toString()
                                   : QString("00:00:00");
  const QTime now = QTime::currentTime();
  const QTime resetTime = QTime::fromString(resetTimeStr, "HH:mm:ss");
  int secondsToReset = now.secsTo(resetTime);
  if (secondsToReset < 0) {
    // 如果已经过了重置时间，计算到第二天的重置时间
    secondsToReset += 24 * 3600;
  }
  const QString formatedTime =
      QTime(0, 0).addSecs(secondsToReset).toString("HH时mm分ss秒");
  ui->countDownLabel->setText(QString("距离重置任务还有 %1").arg(formatedTime));

  // TODO 如果倒计时结束，清楚所有任务状态，并重新开始新的一天任务
}

MainWindow::~MainWindow() { delete ui; }
