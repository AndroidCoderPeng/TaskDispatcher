#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QSystemTrayIcon>

#include "ProcessExecutor.hpp"
#include "TaskExecutor.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  void changeEvent(QEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

public slots:
  void slotSyncSuccess(const QString &message);

  void slotSyncError(const QString &error);

  void slotTaskExecuted(const QDateTime &actualTime, qint32 taskId, int current,
                        int total);

  void slotNextTaskScheduled(int nextIndex, const QDateTime &predictedTime,
                             qint32 nextTaskId);

  void slotConnectStateChanged(ConnectState state);

  void slotDayFinished();

  void slotHolidaySkipped();

  void slotScreenCaptured(const QString &filePath);

  void slotCaptureFailed(const QString &message);

  void slotOpenAppSuccess(const QString &packageName);

  void slotOpenAppFailed(const QString &message);

private:
  Ui::MainWindow *ui;
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *trayMenu = nullptr;
  TaskExecutor *taskExecutorPtr = nullptr;
  ProcessExecutor *processExecutorPtr = nullptr;

  // 任务自动流程中待杀死的包名，空串表示不在自动流程中
  QString pendingKillPackage;

  const QHash<QString, QString> nameToPackage = {
      {"钉钉", "com.alibaba.android.rimet"},
      {"企业微信", "com.tencent.wework"},
      {"飞书", "com.ss.android.lark"},
      {"QQ", "com.tencent.mobileqq"},
      {"微信", "com.tencent.mm"}};
  ConnectState currentState = ConnectState::Disconnected;

  // 自动重连相关
  bool isAutoReconnecting = false;
  int reconnectRetryCount = 0;
  bool disableAutoReconnect = false;

  // ====== 系统托盘 ======
  void setupSystemTray();

  void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

  // ====== 菜单栏 ======
  void onActionImportDataClicked();
  void onActionExportDataClicked();
  void onActionCloseClicked();

  void onActionTargetSettingClicked();
  void onActionEmailSettingClicked();
  void onActionWeWorkSettingClicked();
  void onActionDelayTimeSettingClicked();
  void onActionResetTaskSettingClicked();
  void onActionRandomTimeSettingClicked();
  void onActionSkipHolidaySettingToggled(bool checked);
  void onActionOpenResetTaskSettingToggled(bool checked);
  void onActionOpenRandomTimeSettingToggled(bool checked);

  void onActionDarkThemeToggled(bool checked);

  void onActionSyncDataClicked();

  void onActionWakeUpDeviceClicked();
  void onActionCaptureScreenClicked();
  void onActionScreenOffClicked();
  void onActionOpenTargetAppClicked();
  void onActionKillTargetAppClicked();
  void onActionRestartAdbClicked();

  void onActionTestEmailClicked();
  void onActionTextWxClicked();
  void onActionQuestionClicked();
  void onActionProjectSiteTriggered();
  void onActionAboutTriggered();

  // ====== 按钮 ======
  void onExecuteTaskButtonClicked();
  void onAddTaskButtonClicked();
  void onConnectDeviceButtonClicked();

  // ====== 其他 ======
  void onNotifyMethodChanged();

  void showListWidgetContextMenu(const QPoint &pos);

  void onCustomAction(const QListWidgetItem *item, const QString &message);

  void updateCountDown();

  void resetTaskState();

  void startTaskExecutor();

  void stopTask();

  void updateTaskListWidget();

  void sendMessageToUser(const QString &title, const QString &message);

  void sendMessageToUser(const QByteArray bytes);
};
#endif // MAINWINDOW_HPP
