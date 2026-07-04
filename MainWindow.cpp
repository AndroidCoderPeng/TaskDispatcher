#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AddTaskDialog.hpp"
#include "ChinaHolidayManager.hpp"
#include "ConfigStore.hpp"
#include "EmailSettingDialog.hpp"
#include "ImageProcessor.hpp"
#include "Logger.hpp"
#include "MailSender.hpp"
#include "ProcessExecutor.hpp"
#include "ResetTaskSettingDialog.hpp"
#include "TaskItemWidget.hpp"
#include "TaskStore.hpp"
#include "ToastWidget.hpp"
#include "WxMessageSender.hpp"
#include "WxSettingDialog.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEvent>
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

#include <algorithm>

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
  updateTaskListWidget();

  // 从 ConfigStore 恢复通知方式选中状态
  {
    const QJsonObject saved = ConfigStore::get().load("notifyMethodConfig");
    const QString method =
        saved.contains("method") ? saved["method"].toString() : QString();
    if (method == "wx") {
      ui->wxRadioButton->setChecked(true);
    } else if (method == "email") {
      ui->emailRadioButton->setChecked(true);
    } else {
      Logger::Tag("MainWindow").i("No notify method config found");
    }
  }

  {
    const QJsonObject customMapping =
        ConfigStore::get().load("customAppMapping");
    for (auto it = customMapping.begin(); it != customMapping.end(); ++it) {
      nameToPackage.insert(it.key(), it.value().toString());
    }
  }

  // 初始化系统托盘
  setupSystemTray();

  // 连接顶部菜单信号和槽
  connect(ui->actionImportAppInfo, &QAction::triggered, this,
          &MainWindow::onActionImportAppInfoClicked);
  connect(ui->actionImportData, &QAction::triggered, this,
          &MainWindow::onActionImportDataClicked);
  connect(ui->actionExportData, &QAction::triggered, this,
          &MainWindow::onActionExportDataClicked);
  connect(ui->actionExit, &QAction::triggered, this,
          &MainWindow::onActionCloseClicked);

  connect(ui->actionTargetSetting, &QAction::triggered, this,
          &MainWindow::onActionTargetSettingClicked);
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
  connect(ui->actionOpenRandomTimeSetting, &QAction::toggled, this,
          &MainWindow::onActionOpenRandomTimeSettingToggled);
  connect(ui->actionSkipHolidaySetting, &QAction::toggled, this,
          &MainWindow::onActionSkipHolidaySettingToggled);
  connect(ui->actionOpenResetTaskSetting, &QAction::toggled, this,
          &MainWindow::onActionOpenResetTaskSettingToggled);

  connect(ui->actionDarkTheme, &QAction::toggled, this,
          &MainWindow::onActionDarkThemeToggled);

  connect(ui->actionSyncData, &QAction::triggered, this,
          &MainWindow::onActionSyncDataClicked);

  connect(ui->actionWakeUpDevice, &QAction::triggered, this,
          &MainWindow::onActionWakeUpDeviceClicked);
  connect(ui->actionCaptureScreen, &QAction::triggered, this,
          &MainWindow::onActionCaptureScreenClicked);
  connect(ui->actionScreenOff, &QAction::triggered, this,
          &MainWindow::onActionScreenOffClicked);
  connect(ui->actionOpenTargetApp, &QAction::triggered, this,
          &MainWindow::onActionOpenTargetAppClicked);
  connect(ui->actionKillTargetApp, &QAction::triggered, this,
          &MainWindow::onActionKillTargetAppClicked);
  connect(ui->actionRestartAdb, &QAction::triggered, this,
          &MainWindow::onActionRestartAdbClicked);

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
  connect(ui->executeTaskButton, &QPushButton::clicked, this,
          &MainWindow::onExecuteTaskButtonClicked);
  connect(ui->addTaskButton, &QPushButton::clicked, this,
          &MainWindow::onAddTaskButtonClicked);
  connect(ui->connectDeviceButton, &QPushButton::clicked, this,
          &MainWindow::onConnectDeviceButtonClicked);

  // 连接 RadioButton 信号：通知方式切换
  connect(ui->emailRadioButton, &QRadioButton::toggled, this,
          &MainWindow::onNotifyMethodChanged);
  connect(ui->wxRadioButton, &QRadioButton::toggled, this,
          &MainWindow::onNotifyMethodChanged);

  // 连接任务列表右键菜单信号
  connect(ui->listWidget, &QListWidget::customContextMenuRequested, this,
          &MainWindow::showListWidgetContextMenu);

  // 连接节假日数据同步信号
  const auto holidayManager = ChinaHolidayManager::get();
#ifndef Q_OS_WIN
  connect(holidayManager, &ChinaHolidayManager::signalSslNotFound, this,
          &MainWindow::slotSslNotFound);
#endif
  connect(holidayManager, &ChinaHolidayManager::signalSyncSuccess, this,
          &MainWindow::slotSyncSuccess);
  connect(holidayManager, &ChinaHolidayManager::signalSyncError, this,
          &MainWindow::slotSyncError);

  // 创建任务执行器并连接信号
  taskExecutorPtr = new TaskExecutor(this);
  connect(taskExecutorPtr, &TaskExecutor::signalTaskExecuted, this,
          &MainWindow::slotTaskExecuted);
  connect(taskExecutorPtr, &TaskExecutor::signalNextTaskScheduled, this,
          &MainWindow::slotNextTaskScheduled);
  connect(taskExecutorPtr, &TaskExecutor::signalDayFinished, this,
          &MainWindow::slotDayFinished);
  connect(taskExecutorPtr, &TaskExecutor::signalHolidaySkipped, this,
          &MainWindow::slotHolidaySkipped);

  // 创建进程执行器并连接信号
  processExecutorPtr = new ProcessExecutor(this);
#ifndef Q_OS_WIN
  connect(processExecutorPtr, &ProcessExecutor::signalExecutorNotFound, this,
          &MainWindow::slotExecutorNotFound);
#endif
  connect(processExecutorPtr, &ProcessExecutor::signalConnectStateChanged, this,
          &MainWindow::slotConnectStateChanged);
  connect(processExecutorPtr, &ProcessExecutor::signalScreenCaptured, this,
          &MainWindow::slotScreenCaptured);
  connect(processExecutorPtr, &ProcessExecutor::signalCaptureFailed, this,
          &MainWindow::slotCaptureFailed);
  connect(processExecutorPtr, &ProcessExecutor::signalOpenAppSuccess, this,
          &MainWindow::slotOpenAppSuccess);
  connect(processExecutorPtr, &ProcessExecutor::signalOpenAppFailed, this,
          &MainWindow::slotOpenAppFailed);
}

/// —————————— 内部私有函数 ——————————
void MainWindow::setupSystemTray() {
  // 创建托盘图标
  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(QIcon(":/application.png"));
  trayIcon->setToolTip("任务调度器");

  // 创建托盘右键菜单
  trayMenu = new QMenu(this);
  QAction *showAction = trayMenu->addAction("显示主窗口");
  QAction *quitAction = trayMenu->addAction("退出");

  connect(showAction, &QAction::triggered, this, [this]() {
    showNormal();
    activateWindow();
  });

  connect(quitAction, &QAction::triggered, this, [this]() {
    // 停止任务执行器
    if (taskExecutorPtr && taskExecutorPtr->isRunning()) {
      taskExecutorPtr->stop();
    }
    trayIcon->hide();
    QApplication::quit();
  });

  trayIcon->setContextMenu(trayMenu);

  // 双击托盘图标显示主窗口
  connect(trayIcon, &QSystemTrayIcon::activated, this,
          &MainWindow::onTrayIconActivated);

  trayIcon->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::DoubleClick ||
      reason == QSystemTrayIcon::Trigger) {
    showNormal();
    activateWindow();
  }
}

void MainWindow::onActionImportAppInfoClicked() {
  const QString filePath = QFileDialog::getOpenFileName(this, "导入应用和包名",
                                                        "", "CSV 文件 (*.csv)");
  if (filePath.isEmpty()) {
    return;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "错误", "无法打开文件：" + file.errorString());
    return;
  }

  int importCount = 0;
  while (!file.atEnd()) {
    const QString line = QString::fromUtf8(file.readLine()).trimmed();
    if (line.isEmpty() || line.startsWith('#'))
      continue; // 跳过空行和注释

    const QStringList parts = line.split(',');
    if (parts.size() >= 2) {
      const QString appName = parts[0].trimmed();
      const QString packageName = parts[1].trimmed();
      if (!appName.isEmpty() && !packageName.isEmpty()) {
        nameToPackage.insert(appName, packageName);
        ++importCount;
      }
    }
  }
  file.close();

  QJsonObject customMapping;
  // 默认的四个不需要存，只存用户自定义的
  const QSet<QString> defaultNames = {"钉钉", "企业微信", "飞书", "移动办公M3"};
  for (auto it = nameToPackage.begin(); it != nameToPackage.end(); ++it) {
    if (!defaultNames.contains(it.key())) {
      customMapping.insert(it.key(), it.value());
    }
  }
  ConfigStore::get().save("customAppMapping", customMapping);

  Logger::Tag("MainWindow")
      .dFmt("Imported %d app(s), total: %d", importCount, nameToPackage.size());
  ToastWidget::showInfo(this, QString("成功导入 %1 个应用").arg(importCount));
}

void MainWindow::onActionImportDataClicked() {
  const QString filePath =
      QFileDialog::getOpenFileName(this, "导入数据", "", "配置文件 (*.json)");
  if (filePath.isEmpty()) {
    return;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::critical(this, "错误", "无法打开文件：" + file.errorString());
    return;
  }

  const QByteArray data = file.readAll();
  file.close();

  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    QMessageBox::critical(this, "错误",
                          "JSON 解析失败：" + parseError.errorString());
    return;
  }

  if (!doc.isObject()) {
    QMessageBox::critical(this, "错误",
                          "文件格式不正确，根节点必须是 JSON 对象");
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
  updateTaskListWidget();

  ToastWidget::showInfo(
      this,
      QString("成功导入 %1 项配置，%2 条任务").arg(configCount).arg(taskCount));
}

void MainWindow::onActionExportDataClicked() {
  const auto configs = ConfigStore::get().loadAll();
  const auto tasks = TaskStore::get().loadAll();
  if (configs.isEmpty() && tasks.isEmpty()) {
    QMessageBox::information(this, "提示", "没有数据可以导出");
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
    QMessageBox::critical(this, "错误", "无法打开文件：" + file.errorString());
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
  const auto ret = QMessageBox::question(this, "确认", "确定要退出程序吗？",
                                         QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    QTimer::singleShot(0, this, &QCoreApplication::quit);
  }
}

void MainWindow::onActionTargetSettingClicked() {
  QString defaultValue = "";
  QJsonObject saved = ConfigStore::get().load("targetAppConfig");
  if (saved.contains("targetApp")) {
    defaultValue = saved["targetApp"].toString();
  }

  bool ok = false;
  const QString inputValue = QInputDialog::getText(
      this, "设置目标APP", "请输入想要打开的APP名字或者包名", QLineEdit::Normal,
      defaultValue, &ok);

  if (ok && !inputValue.isEmpty()) {
    // 先查映射表，找不到则直接作为包名使用
    const QString packageName = nameToPackage.value(inputValue, inputValue);
    processExecutorPtr->resolveLauncherActivity(
        packageName, [this, packageName](QString activity) {
          if (activity.isEmpty()) {
            ToastWidget::showError(this, "目标APP配置失败，请检查");
            return;
          }
          QJsonObject obj;
          obj["targetApp"] = packageName;
          obj["launchActivity"] = activity;
          ConfigStore::get().save("targetAppConfig", obj);
          ToastWidget::showInfo(this, "目标APP配置已保存");
        });
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
      QInputDialog::getInt(this, "任务随机时间", "请输入任务随机时间（分钟）",
                           defaultValue, 3, 30, 1, &ok);

  if (ok) {
    QJsonObject obj;
    obj["minutes"] = minutes;
    ConfigStore::get().save("randomTimeConfig", obj);
    ToastWidget::showInfo(this,
                          QString("任务随机时间已设置为 %1 分钟").arg(minutes));
  }
}

void MainWindow::onActionOpenRandomTimeSettingToggled(bool checked) {
  QJsonObject obj;
  obj["openRandomTime"] = checked;
  ConfigStore::get().save("openRandomTimeConfig", obj);
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

void MainWindow::onActionDarkThemeToggled(bool checked) {
  TaskItemWidget::setDarkTheme(checked);
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

  // 只刷新已有 widget 的主题样式，不重建（保留实际时间等状态）
  for (int i = 0; i < ui->listWidget->count(); ++i) {
    auto *widget = dynamic_cast<TaskItemWidget *>(
        ui->listWidget->itemWidget(ui->listWidget->item(i)));
    if (widget) {
      widget->refreshThemeStyle();
    }
  }
}

void MainWindow::onActionSyncDataClicked() {
  ChinaHolidayManager::get()->updateChinaHolidayData();
}

void MainWindow::onActionWakeUpDeviceClicked() {
  processExecutorPtr->wakeUpDevice();
}

void MainWindow::onActionCaptureScreenClicked() {
  processExecutorPtr->captureScreen();
}

void MainWindow::onActionScreenOffClicked() { processExecutorPtr->screenOff(); }

void MainWindow::onActionOpenTargetAppClicked() {
  QJsonObject obj = ConfigStore::get().load("targetAppConfig");
  if (obj.isEmpty()) {
    QMessageBox::warning(this, "警告", "请先配置目标APP");
    return;
  }

  const auto launchActivity = obj["launchActivity"].toString();
  Logger::Tag("MainWindow")
      .dFmt("Opening target app, activity: %s",
            launchActivity.toStdString().c_str());
  processExecutorPtr->openTargetApp(launchActivity);
}

void MainWindow::onActionKillTargetAppClicked() {
  QJsonObject obj = ConfigStore::get().load("targetAppConfig");
  if (obj.isEmpty()) {
    QMessageBox::warning(this, "警告", "请先配置目标APP");
    return;
  }

  const auto packageName = obj["targetApp"].toString();
  Logger::Tag("MainWindow")
      .dFmt("Killing target app: %s", packageName.toStdString().c_str());
  processExecutorPtr->killTargetApp(packageName);
}

void MainWindow::onActionRestartAdbClicked() {
  processExecutorPtr->restartAdb();
}

void MainWindow::onActionTestEmailClicked() {
  QJsonObject obj = ConfigStore::get().load("emailConfig");
  if (obj.isEmpty()) {
    QMessageBox::warning(this, "警告", "请先配置邮箱信息");
    return;
  }

  const QString filePath =
      QFileDialog::getOpenFileName(this, "选择附件", "", "图片 (*.png)");
  if (filePath.isEmpty()) {
    Logger::Tag("MainWindow").i("select image canceled");
    return;
  }

  Logger::Tag("MainWindow")
      .dFmt("selected image: %s", filePath.toStdString().c_str());

  const auto start = QTime::currentTime();
  const auto bytes = ImageProcessor::get()->compressImage(filePath);
  const auto end = QTime::currentTime();
  Logger::Tag("MainWindow")
      .dFmt("compress image cost %d ms", start.msecsTo(end));

  MailSender::get()->sendAttachmentEmail(
      "测试邮件",
      "这是一封来自【任务调度器】的测试邮件，邮件发送功能配置成功！", bytes);
}

void MainWindow::onActionTextWxClicked() {
  QJsonObject obj = ConfigStore::get().load("wxConfig");
  if (obj.isEmpty()) {
    QMessageBox::warning(this, "警告", "请先配置企业微信信息");
    return;
  }

  const QString filePath =
      QFileDialog::getOpenFileName(this, "选择附件", "", "图片 (*.png)");
  if (filePath.isEmpty()) {
    Logger::Tag("MainWindow").i("select image canceled");
    return;
  }

  Logger::Tag("MainWindow")
      .dFmt("selected image: %s", filePath.toStdString().c_str());

  const auto start = QTime::currentTime();
  const auto bytes = ImageProcessor::get()->compressImage(filePath);
  const auto end = QTime::currentTime();
  Logger::Tag("MainWindow")
      .dFmt("compress image cost %d ms", start.msecsTo(end));

  WxMessageSender::get()->sendImageMessageAsync("测试企业微信消息", bytes);
}

void MainWindow::onActionQuestionClicked() {
  QString htmlPath = QApplication::applicationDirPath() + "/html/index.html";
  QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
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
                     "<p>支持 Windows / Linux / Mac 平台。"
                     "<p>Mac平台需自行下载Qt编译链编译。</p>");
}

void MainWindow::onExecuteTaskButtonClicked() {
  // 如果正在运行，则停止
  if (taskExecutorPtr->isRunning()) {
    stopTask();
    Logger::Tag("MainWindow").i("用户手动停止任务调度");
    return;
  }

  // 检查是否有任务
  const auto tasks = TaskStore::get().loadAll();
  if (tasks.isEmpty()) {
    QMessageBox::warning(this, "警告", "没有任务可以执行，请先添加任务");
    return;
  }

  // 加载配置并启动任务执行器
  startTaskExecutor();
  ui->executeTaskButton->setText("停止任务");
  ui->addTaskButton->setEnabled(false);
  ui->listWidget->setEnabled(false);
}

void MainWindow::onAddTaskButtonClicked() {
  AddTaskDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    const auto result = dialog.getInputValue();
    if (result.first) {
      const Task &newTask = result.second;
      const QString newTimeStr = newTask.scheduledTime.toString("HH:mm:ss");

      // 检查是否已存在相同时间点的任务
      const QList<Task> existingTasks = TaskStore::get().loadAll();
      for (const Task &t : existingTasks) {
        if (t.scheduledTime.toString("HH:mm:ss") == newTimeStr) {
          QMessageBox::information(
              this, "提示",
              QString("时间点 %1 已存在，请选择其他时间").arg(newTimeStr));
          return;
        }
      }

      const qint32 newId = TaskStore::get().add(newTask);
      if (newId > 0) {
        // 刷新列表和任务数量
        updateTaskListWidget();
      }
    }
  }
}

void MainWindow::onConnectDeviceButtonClicked() {
  if (currentState == ConnectState::Connected) {
    // 手动断开，禁止后续自动重连
    disableAutoReconnect = true;
    processExecutorPtr->disconnectDevice();
  } else {
    // 手动连接，恢复自动重连能力
    disableAutoReconnect = false;
    isAutoReconnecting = false;
    reconnectRetryCount = 0;
    // 先执行 adb tcpip 5555
    processExecutorPtr->initDebugPort([this](bool result) {
      if (result) {
        // 成功执行 adb tcpip 5555
        QString defaultValue = "";
        QJsonObject saved = ConfigStore::get().load("defaultIpConfig");
        if (saved.contains("defaultIp")) {
          defaultValue = saved["defaultIp"].toString();
        }

        bool ok = false;
        const QString deviceIp =
            QInputDialog::getText(this, "无线连接", "请输入手机的局域网IP地址",
                                  QLineEdit::Normal, defaultValue, &ok);
        if (ok && !deviceIp.isEmpty()) {
          processExecutorPtr->connectDevice(deviceIp);
          QJsonObject obj;
          obj["defaultIp"] = deviceIp;
          ConfigStore::get().save("defaultIpConfig", obj);
        }
      } else {
        QMessageBox::critical(nullptr, "错误",
                              "ADB 初始化失败，请重新插拔手机 USB "
                              "线，并在手机上选择「传输文件」模式（非「仅充电」"
                              "），确保已开启 USB 调试");
      }
    });
  }
}

void MainWindow::updateTaskListWidget() {
  ui->listWidget->clear();
  QList<Task> tasks = TaskStore::get().loadAll();

  // 按时间升序排列
  std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
    return a.scheduledTime < b.scheduledTime;
  });

  // 使用 qAsConst 避免 range-loop detach 警告
  for (const Task &task : qAsConst(tasks)) {
    QListWidgetItem *item = new QListWidgetItem(ui->listWidget);
    item->setData(Qt::UserRole, task.id);

    TaskItemWidget *taskWidget = new TaskItemWidget(task, ui->listWidget);
    item->setSizeHint(taskWidget->sizeHint());
    ui->listWidget->setItemWidget(item, taskWidget);
  }
}

void MainWindow::onNotifyMethodChanged() {
  const QString method = ui->emailRadioButton->isChecked() ? "email" : "wx";
  QJsonObject obj;
  obj["method"] = method;
  ConfigStore::get().save("notifyMethodConfig", obj);
}

void MainWindow::showListWidgetContextMenu(const QPoint &pos) {
  const auto listWidget = qobject_cast<QListWidget *>(sender());
  if (listWidget) {
    const QListWidgetItem *item = listWidget->itemAt(pos);
    if (item == nullptr) {
      return;
    }

    QMenu menu(this);
    const QAction *editAction = menu.addAction("编辑");
    const QAction *deleteAction = menu.addAction("删除");
    const QAction *selectedAction =
        menu.exec(listWidget->viewport()->mapToGlobal(pos));
    if (selectedAction == editAction) {
      onCustomAction(item, "0");
    } else if (selectedAction == deleteAction) {
      onCustomAction(item, "1");
    } else {
      Logger::Tag("MainWindow").w("No action selected in context menu");
    }
  }
}

void MainWindow::onCustomAction(const QListWidgetItem *item,
                                const QString &message) {
  auto *listItem = const_cast<QListWidgetItem *>(item);
  auto *itemWidget =
      qobject_cast<TaskItemWidget *>(ui->listWidget->itemWidget(listItem));
  if (!itemWidget) {
    return;
  }

  const int id = item->data(Qt::UserRole).toInt();
  if (message == "0") {
    // 编辑
    const Task task = TaskStore::get().loadById(id);
    if (task.id <= 0) {
      return;
    }
    AddTaskDialog dialog(this);
    dialog.setTask(task);
    if (dialog.exec() == QDialog::Accepted) {
      const auto result = dialog.getInputValue();
      if (result.first) {
        TaskStore::get().update(result.second);
        updateTaskListWidget();
      }
    }
  } else if (message == "1") {
    // 删除
    const auto ret = QMessageBox::question(this, "确认", "确定要删除该任务吗？",
                                           QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
      TaskStore::get().remove(id);
      updateTaskListWidget();
    }
  } else {
    Logger::Tag("MainWindow")
        .dFmt("Unknown action message: %s", message.toStdString().c_str());
  }
}

#ifndef Q_OS_WIN
void MainWindow::slotSslNotFound() {
  QMessageBox::critical(this, "错误",
                        "未在系统中检测到 OpenSSL 库，请安装 OpenSSL");
}
#endif

void MainWindow::slotSyncSuccess(const QString &message) {
  QMessageBox::information(this, "提示", message);
}

void MainWindow::slotSyncError(const QString &error) {
  QMessageBox::critical(this, "错误", error);
}

void MainWindow::slotTaskExecuted(const QDateTime &actualTime, qint32 taskId,
                                  int current, int total) {
  QJsonObject obj = ConfigStore::get().load("targetAppConfig");
  if (obj.isEmpty()) {
    sendMessageToUser("任务执行失败", "未配置目标APP，无法执行后续步骤");
    return;
  }

  const auto packageName = obj["targetApp"].toString();
  const auto launchActivity = obj["launchActivity"].toString();

  // 记录待杀死的包名，供后续自动流程使用
  pendingKillPackage = packageName;

  // 通过 adb shell monkey 打开目标APP
  processExecutorPtr->openTargetApp(launchActivity);

  Logger::Tag("MainWindow")
      .dFmt("Task executed: taskId=%d, actualTime=%s, current=%d, total=%d",
            taskId, actualTime.toString("HH:mm:ss").toStdString().c_str(),
            current, total);

  // 后续步骤由信号驱动：
  //   slotOpenAppSuccess → 等待 delay 秒 → 截屏
  //   slotScreenCaptured  → 等待 15s → 杀掉目标APP
}

void MainWindow::slotNextTaskScheduled(int nextIndex,
                                       const QDateTime &predictedTime,
                                       qint32 nextTaskId) {
  // 更新任务进度
  ui->taskIndexLabel->setText(QString("%1").arg(nextIndex));

  // 在 listWidget 中查找对应 taskId 的 TaskItemWidget 并设置实际执行时间
  for (int i = 0; i < ui->listWidget->count(); ++i) {
    QListWidgetItem *item = ui->listWidget->item(i);
    auto *widget =
        dynamic_cast<TaskItemWidget *>(ui->listWidget->itemWidget(item));
    if (widget && widget->taskId() == nextTaskId) {
      widget->setActualTime(predictedTime.time());
      break;
    }
  }
}

void MainWindow::slotDayFinished() {
  Logger::Tag("MainWindow").i("当天所有任务执行完毕，重置任务列表实际时间");
  ui->taskIndexLabel->setText("0");
  for (int i = 0; i < ui->listWidget->count(); ++i) {
    auto *widget = dynamic_cast<TaskItemWidget *>(
        ui->listWidget->itemWidget(ui->listWidget->item(i)));
    if (widget) {
      widget->setActualTime(QTime());
    }
  }
  sendMessageToUser("好消息",
                    "当天所有任务执行完毕，休息一下吧~~祝你生活愉快！");
}

void MainWindow::slotHolidaySkipped() {
  Logger::Tag("MainWindow").i("节假日，跳过今日所有任务");
  ui->taskIndexLabel->setText("0");
  for (int i = 0; i < ui->listWidget->count(); ++i) {
    auto *widget = dynamic_cast<TaskItemWidget *>(
        ui->listWidget->itemWidget(ui->listWidget->item(i)));
    if (widget) {
      widget->setActualTime(QTime());
    }
  }
  sendMessageToUser("普天同庆", "今天不上班~，出去玩玩吧！");
}

#ifndef Q_OS_WIN
void MainWindow::slotExecutorNotFound() {
  QMessageBox::warning(this, "缺少 ADB",
                       "未在系统中检测到 adb，请安装 adb：\n"
                       "sudo apt install adb");
}
#endif

void MainWindow::slotConnectStateChanged(ConnectState state) {
  currentState = state;
  processExecutorPtr->getConnectedDeviceName(
      [this, state](const QString &device) {
        if (device.isEmpty()) {
          ui->usbStateView->setText(
              state == ConnectState::Connected ? "设备已连接" : "设备未连接");
        } else {
          ui->usbStateView->setText(state == ConnectState::Connected
                                        ? QString("%1 已连接").arg(device)
                                        : "设备未连接");
        }
      });
  if (state == ConnectState::Connected) {
    disableAutoReconnect = false;
    isAutoReconnecting = false;
    reconnectRetryCount = 0;

    ui->usbIconView->setPixmap(QPixmap(":/usb_connected.png"));
    ui->connectDeviceButton->setText("断开设备");
    ToastWidget::showInfo(this, "设备已通过 WiFi 连接，现在可以拔掉 USB 线了");
  } else if (state == ConnectState::Disconnected) {
    ui->usbIconView->setPixmap(QPixmap(":/usb_disconnected.png"));
    ui->connectDeviceButton->setText("连接设备");

    // 用户手动断开，不做任何自动操作
    if (disableAutoReconnect) {
      return;
    }

    // 已在自动重连中，忽略重复的断开通知
    if (isAutoReconnecting)
      return;

    const QJsonObject saved = ConfigStore::get().load("defaultIpConfig");
    if (!saved.contains("defaultIp")) {
      return;
    }

    isAutoReconnecting = true;
    reconnectRetryCount = 0;
    const QString ip = saved["defaultIp"].toString();
    Logger::Tag("MainWindow")
        .dFmt("设备断开，尝试自动重连: %s", ip.toStdString().c_str());
    processExecutorPtr->connectDevice(ip);
  } else if (state == ConnectState::ConnectFailed) {
    // 非自动重连场景（用户手动连接失败）
    if (!isAutoReconnecting) {
      return;
    }

    reconnectRetryCount++;
    if (reconnectRetryCount < 3) {
      const QString ip =
          ConfigStore::get().load("defaultIpConfig")["defaultIp"].toString();
      Logger::Tag("MainWindow")
          .dFmt("自动重连失败，第 %d 次重试: %s", reconnectRetryCount,
                ip.toStdString().c_str());
      processExecutorPtr->connectDevice(ip);
    } else {
      Logger::Tag("MainWindow").w("自动重连 3 次均失败");
      isAutoReconnecting = false;
      sendMessageToUser(
          "设备连接失败",
          "设备自动重连 3 次均失败，请检查设备网络连接后手动重连");
    }
  }
}

void MainWindow::slotScreenCaptured(const QString &filePath) {
  Logger::Tag("MainWindow")
      .dFmt("截屏已保存: %s", filePath.toStdString().c_str());
  const auto bytes = ImageProcessor::get()->compressImage(filePath);
  sendMessageToUser(bytes);

  // 如果是任务自动流程，等待 15s 后杀掉目标APP
  if (!pendingKillPackage.isEmpty()) {
    const QString package = pendingKillPackage;
    pendingKillPackage.clear();

    Logger::Tag("MainWindow")
        .dFmt("截屏完成，等待 15 秒后杀掉应用: %s",
              package.toStdString().c_str());

    QTimer::singleShot(15000, this, [this, package]() {
      Logger::Tag("MainWindow")
          .dFmt("15 秒到，杀死应用: %s", package.toStdString().c_str());
      processExecutorPtr->killTargetApp(package);
    });
  }
}

void MainWindow::slotCaptureFailed(const QString &message) {
  ToastWidget::showError(this, message);
  sendMessageToUser("截屏失败通知", message);
  pendingKillPackage.clear();
}

void MainWindow::slotOpenAppSuccess() {
  if (pendingKillPackage.isEmpty()) {
    Logger::Tag("MainWindow").i("手动打开应用，不触发后续截屏+杀App流程");
    return;
  }

  // 读取 delay 配置，延迟后截屏
  int delaySeconds = 30;
  {
    const QJsonObject saved = ConfigStore::get().load("delayTimeConfig");
    if (saved.contains("seconds")) {
      delaySeconds = saved["seconds"].toInt();
    }
  }

  Logger::Tag("MainWindow")
      .dFmt("应用已打开，等待 %d 秒后截屏...", delaySeconds);

  QTimer::singleShot(delaySeconds * 1000, this, [this]() {
    Logger::Tag("MainWindow").i("延迟时间到，开始截屏");
    processExecutorPtr->captureScreen();
  });
}

void MainWindow::slotOpenAppFailed(const QString &message) {
  ToastWidget::showError(this, message);
  pendingKillPackage.clear();
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

  // 如果倒计时结束，清除所有任务状态，并重新开始新的一天任务
  if (secondsToReset == 0) {
    resetTaskState();
  }
}

void MainWindow::resetTaskState() {
  Logger::Tag("MainWindow").i("开始重置任务状态...");

  if (taskExecutorPtr && taskExecutorPtr->isRunning()) {
    taskExecutorPtr->stop();
    Logger::Tag("MainWindow").i("已停止任务执行器");
  }

  updateTaskListWidget();

  // 重新启动任务执行器，开始新的一天任务
  if (taskExecutorPtr) {
    startTaskExecutor();
  }
}

// 从配置中加载参数并启动任务执行器
void MainWindow::startTaskExecutor() {
  // 读取节假日跳过配置
  bool skipHoliday = false;
  {
    const QJsonObject cfg = ConfigStore::get().load("skipHolidayConfig");
    if (cfg.contains("skipHoliday")) {
      skipHoliday = cfg["skipHoliday"].toBool();
    }
  }

  // 读取随机时间配置
  bool randomEnabled = true;
  int randomMinutes = 5;
  {
    const QJsonObject cfg = ConfigStore::get().load("openRandomTimeConfig");
    if (cfg.contains("openRandomTime")) {
      randomEnabled = cfg["openRandomTime"].toBool();
    }
  }
  {
    const QJsonObject cfg = ConfigStore::get().load("randomTimeConfig");
    if (cfg.contains("minutes")) {
      randomMinutes = cfg["minutes"].toInt();
    }
  }

  // 读取任务重置时间，默认 0 点
  QTime resetTime(0, 0);
  {
    const QJsonObject cfg = ConfigStore::get().load("resetTaskConfig");
    if (cfg.contains("time")) {
      resetTime = QTime::fromString(cfg["time"].toString(), "HH:mm:ss");
    }
  }

  // 配置并启动执行器
  taskExecutorPtr->setSkipHoliday(skipHoliday);
  taskExecutorPtr->setRandomTimeConfig(randomEnabled, randomMinutes);
  taskExecutorPtr->setResetTime(resetTime);
  taskExecutorPtr->start();

  Logger::Tag("MainWindow")
      .dFmt("执行器配置: 随机=%s (%d分钟), 跳过节假日=%s, 重置时间=%s",
            randomEnabled ? "是" : "否", randomMinutes,
            skipHoliday ? "是" : "否",
            resetTime.toString("HH:mm").toStdString().c_str());
  sendMessageToUser("任务启动通知", "任务执行器已启动，开始新的一天任务");
}

void MainWindow::stopTask() {
  taskExecutorPtr->stop();
  ui->executeTaskButton->setText("执行任务");
  ui->addTaskButton->setEnabled(true);
  ui->listWidget->setEnabled(true);
  ui->taskIndexLabel->setText("0");

  // 清空所有任务的实际执行时间
  for (int i = 0; i < ui->listWidget->count(); ++i) {
    QListWidgetItem *item = ui->listWidget->item(i);
    auto *widget =
        dynamic_cast<TaskItemWidget *>(ui->listWidget->itemWidget(item));
    if (widget) {
      widget->setActualTime(QTime());
    }
  }
}

void MainWindow::sendMessageToUser(const QString &title,
                                   const QString &message) {
  Logger::Tag("MainWindow")
      .box()
      .add(title.toStdString().c_str())
      .add(message.toStdString().c_str())
      .print();
  const QJsonObject saved = ConfigStore::get().load("notifyMethodConfig");
  const QString method =
      saved.contains("method") ? saved["method"].toString() : QString();
  if (method == "wx") {
    QJsonObject obj = ConfigStore::get().load("wxConfig");
    if (obj.isEmpty()) {
      Logger::Tag("MainWindow").w("企业微信未配置");
      return;
    }
    WxMessageSender::get()->sendMessageAsync(title, message);
  } else {
    QJsonObject obj = ConfigStore::get().load("emailConfig");
    if (obj.isEmpty()) {
      Logger::Tag("MainWindow").w("邮箱信息未配置");
      return;
    }
    MailSender::get()->sendEmail(title, message);
  }

  // 通知完用户后关闭屏幕，节省电量，不必手动调用，手机有自动息屏设置
  // processExecutorPtr->screenOff();
}

void MainWindow::sendMessageToUser(const QByteArray bytes) {
  const QJsonObject saved = ConfigStore::get().load("notifyMethodConfig");
  const QString method =
      saved.contains("method") ? saved["method"].toString() : QString();
  if (method == "wx") {
    QJsonObject obj = ConfigStore::get().load("wxConfig");
    if (obj.isEmpty()) {
      Logger::Tag("MainWindow").w("企业微信未配置");
      return;
    }
    WxMessageSender::get()->sendImageMessageAsync("截屏结果通知", bytes);
  } else {
    QJsonObject obj = ConfigStore::get().load("emailConfig");
    if (obj.isEmpty()) {
      Logger::Tag("MainWindow").w("邮箱信息未配置");
      return;
    }
    MailSender::get()->sendAttachmentEmail("截屏结果通知",
                                           "结果见附件，请注意查收", bytes);
  }

  // 通知完用户后关闭屏幕，节省电量，不必手动调用，手机有自动息屏设置
  // processExecutorPtr->screenOff();
}

void MainWindow::changeEvent(QEvent *event) {
  if (event->type() == QEvent::WindowStateChange) {
    if (isMinimized()) {
      hide();
      if (trayIcon) {
        trayIcon->showMessage("任务调度器", "程序已最小化到系统托盘",
                              QSystemTrayIcon::Information, 2000);
      }
      event->ignore();
      return;
    }
  }
  QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("关闭窗口");
  msgBox.setText("请选择操作");
  msgBox.setIcon(QMessageBox::Question);

  QPushButton *trayBtn =
      msgBox.addButton("最小化到托盘", QMessageBox::AcceptRole);
  msgBox.addButton("直接退出", QMessageBox::DestructiveRole);

  msgBox.setDefaultButton(trayBtn);
  msgBox.exec();

  if (msgBox.clickedButton() == trayBtn) {
    hide();
    if (trayIcon) {
      trayIcon->showMessage("任务调度器", "程序仍在后台运行",
                            QSystemTrayIcon::Information, 2000);
    }
    event->ignore();
  } else {
    if (taskExecutorPtr && taskExecutorPtr->isRunning()) {
      taskExecutorPtr->stop();
    }
    if (trayIcon) {
      trayIcon->hide();
    }
    event->accept();
  }
}

MainWindow::~MainWindow() { delete ui; }
