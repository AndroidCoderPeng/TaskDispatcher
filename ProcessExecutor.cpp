#include "ProcessExecutor.hpp"

#include "Logger.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>

#include <QTimer>
#include <QtGlobal>

ProcessExecutor::ProcessExecutor(QObject *parent) : QObject(parent) {
  Logger::Tag("ProcessExecutor")
      .dFmt("Using adb path: %s", adb().toStdString().c_str());
}

QString ProcessExecutor::adb() {
#ifdef Q_OS_WIN
  return QCoreApplication::applicationDirPath() + "/tool/windows/adb.exe";
#else
  return "adb";
#endif
}

void ProcessExecutor::initDebugPort(std::function<void(bool)> callback) {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [process, callback](int exitCode, QProcess::ExitStatus) {
            process->deleteLater();

            if (exitCode != 0) {
              const QString err = process->readAllStandardError().trimmed();
              Logger::Tag("ProcessExecutor")
                  .eFmt("Failed to init wifi adb port: %s",
                        err.toStdString().c_str());
              if (callback) {
                callback(false);
              }
              return;
            }

            Logger::Tag("ProcessExecutor")
                .i("WiFi adb port 5555 initialized successfully");
            if (callback) {
              callback(true);
            }
          });
  process->start(adb(), {"tcpip", "5555"});
}

void ProcessExecutor::restartAdb() {
  Logger::Tag("ProcessExecutor").i("Restarting ADB server...");
  QProcess *kill = new QProcess(this);
  connect(kill, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this, [this, kill](int, QProcess::ExitStatus) {
            kill->deleteLater();

            // kill-server 完成后启动 server
            QProcess *start = new QProcess(this);
            connect(
                start,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, start](int exitCode, QProcess::ExitStatus) {
                  start->deleteLater();

                  if (exitCode != 0) {
                    const QString err = start->readAllStandardError().trimmed();
                    Logger::Tag("ProcessExecutor")
                        .eFmt("Failed to restart ADB server: %s",
                              err.toStdString().c_str());
                  } else {
                    Logger::Tag("ProcessExecutor")
                        .i("ADB server restarted successfully");
                  }
                });
            start->start(adb(), {"start-server"});
          });
  kill->start(adb(), {"kill-server"});
}

void ProcessExecutor::connectDevice(const QString &deviceIp) {
  emit signalConnectStateChanged(ConnectState::Connecting);
  Logger::Tag("ProcessExecutor")
      .iFmt("Connecting to device: %s", deviceIp.toStdString().c_str());

  QProcess *process = new QProcess(this);
  connect(
      process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
      this, [this, process, deviceIp](int exitCode, QProcess::ExitStatus) {
        process->deleteLater();

        if (exitCode != 0) {
          const QString err = process->readAllStandardError().trimmed();
          Logger::Tag("ProcessExecutor")
              .eFmt("Failed to connect to device %s: %s",
                    deviceIp.toStdString().c_str(), err.toStdString().c_str());
          emit signalConnectStateChanged(ConnectState::ConnectFailed);
          return;
        }

        Logger::Tag("ProcessExecutor")
            .iFmt("Successfully connected to device: %s",
                  deviceIp.toStdString().c_str());
        emit signalConnectStateChanged(ConnectState::Connected);
      });
  process->start(adb(), {"connect", deviceIp});
}

void ProcessExecutor::getConnectedDeviceName(
    std::function<void(QString)> callback) {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [process, callback](int exitCode, QProcess::ExitStatus) {
            process->deleteLater();

            if (exitCode != 0) {
              if (callback) {
                callback("");
              }
              return;
            }

            if (callback) {
              const QString output = process->readAllStandardOutput().trimmed();
              callback(output);
            }
          });
  process->start(adb(), {"shell", "getprop", "ro.product.brand"});
}

void ProcessExecutor::disconnectDevice() {
  Logger::Tag("ProcessExecutor").i("Disconnecting device...");

  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this, process](int exitCode, QProcess::ExitStatus) {
            process->deleteLater();

            if (exitCode != 0) {
              const QString err = process->readAllStandardError().trimmed();
              Logger::Tag("ProcessExecutor")
                  .eFmt("Failed to disconnect device: %s",
                        err.toStdString().c_str());
            } else {
              Logger::Tag("ProcessExecutor").i("Device disconnected");
            }

            // 无论 adb disconnect 是否成功，逻辑状态都变为 Disconnected
            emit signalConnectStateChanged(ConnectState::Disconnected);
          });
  process->start(adb(), {"disconnect"});
}

void ProcessExecutor::startPeriodicCheck(int intervalMs) {
  if (!chekTimerPtr) {
    chekTimerPtr = new QTimer(this);
    connect(chekTimerPtr, &QTimer::timeout, this,
            &ProcessExecutor::checkConnectState);
  }
  chekTimerPtr->start(intervalMs);
  Logger::Tag("ProcessExecutor")
      .dFmt("Periodic connection check started, interval=%d ms", intervalMs);
}

void ProcessExecutor::checkConnectState() {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this, process](int exitCode, QProcess::ExitStatus) {
            process->deleteLater();

            if (exitCode != 0) {
              Logger::Tag("ProcessExecutor")
                  .w("Failed to check device connection state");
              emit signalConnectStateChanged(ConnectState::Disconnected);
              return;
            }

            const QString output = process->readAllStandardOutput().trimmed();
            // "adb devices" 输出中，有 "\tdevice" 的行表示设备已连接
            const bool connected = output.contains("\tdevice");

            if (!connected) {
              Logger::Tag("ProcessExecutor").w("Device disconnected detected");
              emit signalConnectStateChanged(ConnectState::Disconnected);
            }
          });
  process->start(adb(), {"devices"});
}

void ProcessExecutor::stopPeriodicCheck() {
  if (chekTimerPtr) {
    chekTimerPtr->stop();
    Logger::Tag("ProcessExecutor").i("Periodic connection check stopped");
  }
}

void ProcessExecutor::wakeUpDevice() {
  // 亮屏
  QProcess *wake = new QProcess(this);
  connect(wake, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          wake, &QProcess::deleteLater);
  wake->start(adb(), {"shell", "input", "keyevent", "KEYCODE_WAKEUP"});
  Logger::Tag("ProcessExecutor").i("Waking up device...");

  // 亮屏后延迟一下，等锁屏界面显示出来，再做上滑解锁
  QTimer::singleShot(1000, this, [this]() {
    // 获取屏幕分辨率
    QProcess *wm = new QProcess(this);
    connect(wm, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, wm](int, QProcess::ExitStatus) {
              wm->deleteLater();
              const QString output = wm->readAllStandardOutput().trimmed();
              Logger::Tag("ProcessExecutor")
                  .dFmt("Screen size: %s", output.toStdString().c_str());

              int width = 720;
              int height = 1280;
              // 解析 "Physical size: 1080x2400" 或 "1080x2400"
              const QString sizeStr = output.section(':', -1).trimmed();
              const QStringList wh = sizeStr.split('x');
              if (wh.size() == 2) {
                width = wh[0].toInt();
                height = wh[1].toInt();
              }

              // 从底部向上滑动解锁
              const int x = width / 2;
              const int yFrom = height * 4 / 5;
              const int yTo = height / 5;
              QProcess *swipe = new QProcess(this);
              connect(
                  swipe,
                  QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                  this, [this, swipe](int, QProcess::ExitStatus) {
                    swipe->deleteLater();
                    Logger::Tag("ProcessExecutor")
                        .d("Device woken up, waiting 3s for stabilization...");
                    // 等 3 秒让屏幕稳定，再通知调用方
                    QTimer::singleShot(
                        3000, this, [this]() { emit signalDeviceWokenUp(); });
                  });
              swipe->start(adb(), {"shell", "input", "swipe",
                                   QString::number(x), QString::number(yFrom),
                                   QString::number(x), QString::number(yTo)});
              Logger::Tag("ProcessExecutor")
                  .dFmt("Swipe unlock: %d,%d -> %d,%d", x, yFrom, x, yTo);
            });
    wm->start(adb(), {"shell", "wm", "size"});
  });
}

void ProcessExecutor::captureScreen() {
  wakeUpDevice();

  // 等待设备唤醒完成后再截屏
  QMetaObject::Connection *captureConn = new QMetaObject::Connection;
  *captureConn = connect(
      this, &ProcessExecutor::signalDeviceWokenUp, this, [this, captureConn]() {
        disconnect(*captureConn);
        delete captureConn;

        const QString captureDir =
            QCoreApplication::applicationDirPath() + "/capture";
        QDir dir(captureDir);
        if (!dir.exists()) {
          dir.mkpath(".");
        }

        const QString fileName =
            QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
        const QString filePath = captureDir + "/" + fileName;

        QProcess *process = new QProcess(this);
        connect(
            process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process, filePath](int exitCode, QProcess::ExitStatus) {
              process->deleteLater();

              if (exitCode != 0) {
                const QString err = process->readAllStandardError().trimmed();
                QString tip;
                if (err.contains("no devices")) {
                  tip = "未检测到已连接的 Android 设备，请检查 USB 连接或 adb "
                        "状态";
                } else if (err.contains("more than one device")) {
                  tip = "检测到多个设备连接，请仅保留一台设备";
                } else {
                  tip = err;
                }
                emit signalCaptureFailed(tip);
                return;
              }

              QFile file(filePath);
              if (!file.open(QIODevice::WriteOnly)) {
                emit signalCaptureFailed("无法写入截图文件: " + filePath);
                return;
              }
              file.write(process->readAllStandardOutput());
              file.close();

              emit signalScreenCaptured(filePath);
            });
        process->start(adb(), {"exec-out", "screencap", "-p"});
      });
}

void ProcessExecutor::openTargetApp(const QString &packageName) {
  wakeUpDevice();

  // 等待设备唤醒完成后再打开 App
  QMetaObject::Connection *openConn = new QMetaObject::Connection;
  *openConn = connect(
      this, &ProcessExecutor::signalDeviceWokenUp, this,
      [this, openConn, packageName]() {
        disconnect(*openConn);
        delete openConn;

        QStringList args;
        args << "shell" << "monkey"
             << "-p" << packageName << "-c"
             << "android.intent.category.LAUNCHER" << QString::number(1);

        QProcess *process = new QProcess(this);
        connect(
            process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process, packageName](int exitCode,
                                         QProcess::ExitStatus exitStatus) {
              process->deleteLater();

              if (exitStatus == QProcess::CrashExit || exitCode != 0) {
                const QString err = process->readAllStandardError().trimmed();
                QString tip;
                if (err.contains("no devices") ||
                    err.contains("device not found")) {
                  tip = "未检测到已连接的 Android 设备，请检查 USB 连接或 adb "
                        "状态";
                } else if (err.contains("more than one device")) {
                  tip = "检测到多个设备连接，请指定设备序列号或仅保留一台设备";
                } else if (err.contains("Permission denied")) {
                  tip = "权限不足，请确认 adb 有执行权限";
                } else {
                  tip = err;
                }
                emit signalOpenAppFailed(tip);
                return;
              }

              emit signalOpenAppSuccess(packageName);
            });

        process->start(adb(), args);
      });
}

void ProcessExecutor::killTargetApp(const QString &packageName) {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          process, &QProcess::deleteLater);
  process->start(adb(), {"shell", "am", "force-stop", packageName});
}

void ProcessExecutor::screenOff() {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          process, &QProcess::deleteLater);
  process->start(adb(), {"shell", "input", "keyevent", "KEYCODE_SLEEP"});
}