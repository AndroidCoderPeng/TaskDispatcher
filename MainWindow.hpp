#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

#include "GlobalDefinition.hpp"
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

private slots:
  void onTaskExecuted(const QDateTime &actualTime, qint32 taskId, int current,
                      int total);

  void onNextTaskScheduled(int nextIndex, const QDateTime &predictedTime,
                           qint32 nextTaskId);

  void onDayFinished();

  void onHolidaySkipped();

  void onCycleReset();

private:
  Ui::MainWindow *ui;
  QString targetPackage = "com.alibaba.android.rimet";
  TaskExecutor *executorPtr = nullptr;

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

  void updateCountDown();

  void updateTaskListWidget();

  void captureScreen();

  void killTargetApp();

  void updateExecuteButtonStyle(bool running);
};
#endif // MAINWINDOW_HPP
