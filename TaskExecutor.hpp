#ifndef TASKEXECUTOR_HPP
#define TASKEXECUTOR_HPP

#include <QList>
#include <QObject>
#include <QTime>
#include <QTimer>

#include "GlobalDefinition.hpp"

class TaskExecutor : public QObject {
  Q_OBJECT
public:
  explicit TaskExecutor(QObject *parent = nullptr);

  void setTasks(const QList<Task> &newTasks);
  void setSkipHoliday(bool skip);
  void setRandomTimeConfig(bool enabled, int maxMinutes = 5);
  void setResetTime(const QTime &time);

  void addTask(const Task &task);
  void clearTasks();

  void start();
  void stop();

  bool isRunning() const;

signals:
  // 单个任务节点触发
  void signalTaskExecuted(const QDateTime &time);

  // 当天所有任务执行完毕（执行器仍在运行，等待下一天周期）
  void signalDayFinished();

  // 节假日导致当日跳过（执行器仍在运行，等待下一天周期）
  void signalHolidaySkipped();

  // 新一天周期开始，任务已重新加载
  void signalCycleReset();

private slots:
  void executeNextTask();
  void startNewCycle();

private:
  int calculateDelayToNextMs() const;
  int calculateDelayToResetMs() const;

  // 重新从 TaskStore 加载任务，重置索引，排序
  void reloadTasks();

  QList<Task> tasks;
  QTimer timer;
  int currentIndex = 0;
  bool running = false;
  bool skipHoliday = false;
  bool randomEnabled = false;
  int randomMaxMinutes = 5;
  QTime resetTime = QTime(0, 0); // 默认 0 点重置
};

#endif // TASKEXECUTOR_HPP
