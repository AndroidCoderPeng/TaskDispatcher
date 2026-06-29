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
  // 单个任务节点触发：实际执行时间、任务ID、当前进度(1-based)、任务总数
  void signalTaskExecuted(const QDateTime &actualTime, qint32 taskId,
                          int current, int total);

  // 下一个待执行任务：1-based 索引、预测执行时间、任务ID
  void signalNextTaskScheduled(int nextIndex, const QDateTime &predictedTime,
                               qint32 nextTaskId);

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

  // 跳过启动时已过时间的任务，返回实际起始索引
  int skipPastTasks(int startIndex) const;

  // 发射 signalNextTaskScheduled（当前 currentIndex 必须已指向下一任务且
  // currentRandomOffset 已就绪）
  void emitNextTaskInfo();

  QList<Task> tasks;
  QTimer timer;
  int currentIndex = 0;
  bool running = false;
  bool skipHoliday = false;
  bool randomEnabled = false;
  int randomMaxMinutes = 5;
  int currentRandomOffset = 0;   // 当前任务的随机偏移（分钟）
  QTime resetTime = QTime(0, 0); // 默认 0 点重置
};

#endif // TASKEXECUTOR_HPP
