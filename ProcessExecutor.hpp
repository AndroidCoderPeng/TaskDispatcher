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

  void openTargetApp(const QString &packageName);

  void killTargetApp(const QString &packageName);

signals:
  void signalScreenCaptured(const QString &filePath);

  void signalCaptureFailed(const QString &message);

  void signalOpenAppSuccess(const QString &packageName);

  void signalOpenAppFailed(const QString &message);
};

#endif // PROCESSEXECUTOR_HPP
