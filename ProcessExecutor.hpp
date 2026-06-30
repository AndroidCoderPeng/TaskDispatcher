#ifndef PROCESSEXECUTOR_HPP
#define PROCESSEXECUTOR_HPP

#include <QObject>
#include <QProcess>
#include <QString>

class QProcess;

class ProcessExecutor : public QObject {
  Q_OBJECT
public:
  explicit ProcessExecutor(QObject *parent = nullptr);

  void captureScreen();

  // 通过 adb shell monkey 打开目标应用
  // @param packageName 应用包名
  // @param eventCount  monkey 注入事件数，默认 1 即可启动应用
  void openTargetApp(const QString &packageName);

  void killTargetApp(const QString &packageName);

signals:
  void signalScreenCaptured(const QString &filePath);

  void signalCaptureFailed(const QString &message);

  void signalOpenAppSuccess(const QString &packageName);

  void signalOpenAppFailed(const QString &message);

private:
  QString targetPackage = "com.alibaba.android.rimet";
};

#endif // PROCESSEXECUTOR_HPP
