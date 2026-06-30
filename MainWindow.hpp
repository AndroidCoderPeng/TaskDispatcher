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

public slots:
  void slotSyncSuccess(const QString &message);

  void slotSyncError(const QString &error);

  void slotTaskExecuted(const QDateTime &actualTime, qint32 taskId, int current,
                        int total);

  void slotNextTaskScheduled(int nextIndex, const QDateTime &predictedTime,
                             qint32 nextTaskId);

  void slotDayFinished();

  void slotHolidaySkipped();

  void slotCycleReset();

  void slotScreenCaptured(const QString &filePath);

  void slotCaptureFailed(const QString &message);

  void slotOpenAppSuccess(const QString &packageName);

  void slotOpenAppFailed(const QString &message);

private:
  Ui::MainWindow *ui;
  TaskExecutor *taskExecutorPtr = nullptr;
  ProcessExecutor *processExecutorPtr = nullptr;

  const QHash<QString, QString> nameToPackage = {
      {"钉钉", "com.alibaba.android.rimet"},
      {"企业微信", "com.tencent.wework"},
      {"飞书", "com.ss.android.lark"},
      {"QQ", "com.tencent.mobileqq"},
      {"微信", "com.tencent.mm"},
      {"抖音", "com.ss.android.ugc.aweme"}};

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

  void sendMessageToUser(const QString &title, const QString &message);

  void sendMessageToUser(const QByteArray bytes);
};
#endif // MAINWINDOW_HPP
