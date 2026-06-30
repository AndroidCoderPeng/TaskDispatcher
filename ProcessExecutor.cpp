#include "ProcessExecutor.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QtGlobal>

ProcessExecutor::ProcessExecutor(QObject *parent) : QObject(parent) {}

void ProcessExecutor::captureScreen() {
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

        // 发射成功信号，返回图片路径
        emit signalScreenCaptured(filePath);
      });
  process->start("adb", {"exec-out", "screencap", "-p"});
}

void ProcessExecutor::openTargetApp(const QString &packageName) {
  QStringList args;

  args << "shell" << "monkey"
       << "-p" << packageName << "-c" << "android.intent.category.LAUNCHER"
       << QString::number(1);

  QProcess *process = new QProcess(this);
  connect(
      process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
      this,
      [this, process, packageName](int exitCode,
                                   QProcess::ExitStatus exitStatus) {
        process->deleteLater();

        if (exitStatus == QProcess::CrashExit || exitCode != 0) {
          const QString err = process->readAllStandardError().trimmed();
          QString tip;
          if (err.contains("no devices") || err.contains("device not found")) {
            tip = "未检测到已连接的 Android 设备，请检查 USB 连接或 adb 状态";
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

  process->start("adb", args);
}

void ProcessExecutor::killTargetApp(const QString &packageName) {
  QProcess *process = new QProcess(this);
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          process, &QProcess::deleteLater);
  process->start("adb", {"shell", "am", "force-stop", packageName});
}
