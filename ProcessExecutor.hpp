#ifndef PROCESSEXECUTOR_HPP
#define PROCESSEXECUTOR_HPP

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

#include "GlobalDefinition.hpp"

class ProcessExecutor : public QObject {
  Q_OBJECT
public:
  explicit ProcessExecutor(QObject *parent = nullptr);

  void initDebugPort(std::function<void(bool)> callback);

  void restartAdb();

  void disconnectDevice();

  void resolveLauncherActivity(
      const QString &packageName,
      std::function<void(const QString &activity)> callback);

  /// -------------- 以下函数均需要指定设备，否则不生效 --------------

  void connectDevice(const QString &deviceIp);

  void getConnectedDeviceName(std::function<void(QString)> callback);

  void wakeUpDevice();

  void captureScreen();

  void openTargetApp(const QString activity);

  void killTargetApp(const QString &packageName);

  void screenOff();

signals:
  void signalExecutorNotFound();

  void signalConnectStateChanged(ConnectState state);

  void signalDeviceWokenUp();

  void signalScreenCaptured(const QString &filePath);

  void signalCaptureFailed(const QString &message);

  void signalOpenAppSuccess();

  void signalOpenAppFailed(const QString &message);

private:
  QTimer *chekTimerPtr = nullptr;
  QString connectedDevice;
  // 追踪上次状态
  ConnectState lastKnownState = ConnectState::Disconnected;

  QString selectExecutor();

  // 条件化添加 -s <device>，当 connectedDevice 为空时不添加，让 adb 自动选设备
  QStringList appendArgs(const QStringList &args) const;

  void startPeriodicCheck(int intervalMs = 3000);

  void checkConnectState();

  void stopPeriodicCheck();
};

#endif // PROCESSEXECUTOR_HPP
