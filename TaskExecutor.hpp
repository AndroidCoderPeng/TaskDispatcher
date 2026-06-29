#ifndef TASKEXECUTOR_HPP
#define TASKEXECUTOR_HPP

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include "GlobalDefinition.hpp"

class TaskExecutor : public QObject {
  Q_OBJECT
public:
  explicit TaskExecutor(QObject *parent = nullptr);

  void setTasks(const QList<Task> &newTasks);

  void addTask(const QString &time);

  void clearTasks();

  void start();

  void stop();

  bool isRunning() const;

signals:
  void signalTaskExecuted(const QDateTime &time);

  void signalTaskFinished();

private slots:
  void executeNextTask();

private:
  QList<Task> tasks;
  QTimer timer;
  int currentIndex;
  bool running;
};

#endif // TASKEXECUTOR_HPP
