#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QListWidget>
#include <QMainWindow>

#include "GlobalDefinition.hpp"
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

  void bindIpAddresses(const QList<QString> &ips);

public slots:
  void slotNoClient();

  void slotServerStateChanged(const WebSocketState &state);

  void slotDataReceived(const QString &message);

  void slotScreenCaptured(const QString &filePath);

  void slotCaptureFailed(const QString &message);

private:
  Ui::MainWindow *ui;
  QString targetPackage = "com.alibaba.android.rimet";
  TaskExecutor *taskExecutorPtr = nullptr;
  ProcessExecutor *processExecutorPtr = nullptr;

  // ====== 菜单栏 ======
  void onActionImportDataClicked();
  void onActionExportDataClicked();
  void onActionCloseClicked();

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

  void onActionCaptureScreenClicked();
  void onActionOpenTargetAppClicked();
  void onActionKillTargetAppClicked();

  void onActionTestEmailClicked();
  void onActionTextWxClicked();
  void onActionProjectSiteTriggered();
  void onActionAboutTriggered();

  void onExecuteTaskButtonClicked();
  void onAddTaskButtonClicked();
  void onOpenSocketButtonClicked();

  void onNotifyMethodChanged();

  void showListWidgetContextMenu(const QPoint &pos);

  void onCustomAction(const QListWidgetItem *item, const QString &message);

  void updateCountDown();

  void resetTaskState();

  void startTaskExecutor();

  void stopTask();

  void updateTaskListWidget();

  void captureScreen();

  void killTargetApp();
};
#endif // MAINWINDOW_HPP
