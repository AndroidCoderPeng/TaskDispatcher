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

  void connectDevice(const QString &deviceIp);

  void getConnectedDeviceName(std::function<void(QString)> callback);

  void disconnectDevice();

  void startPeriodicCheck(int intervalMs = 5000);

  void stopPeriodicCheck();

  void wakeUpDevice();

  void captureScreen();

  void openTargetApp(const QString &packageName);

  void killTargetApp(const QString &packageName);

  void screenOff();

signals:
  void signalConnectStateChanged(ConnectState state);

  void signalDeviceWokenUp();

  void signalScreenCaptured(const QString &filePath);

  void signalCaptureFailed(const QString &message);

  void signalOpenAppSuccess(const QString &packageName);

  void signalOpenAppFailed(const QString &message);

private:
  QTimer *chekTimerPtr = nullptr;

  QString adb();

  void checkConnectState();
};

#endif // PROCESSEXECUTOR_HPP
