#ifndef TASKEXECUTOR_HPP
#define TASKEXECUTOR_HPP

#include <QHash>
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

private slots:
  void onTimerTimeout();

private:
  /// ==================== 辅助方法 ====================

  // 一次性预计算所有任务的随机偏移（在 start/startNewCycle 时调用）
  void precomputeRandomOffsets();

  // 获取任务的随机偏移秒数（从缓存读取，保证一致性）
  int randomOffset(qint32 taskId) const;

  // 计算到下一个任务的延迟（毫秒），包含第二层逻辑：随机时间波动
  int calculateDelayToNextMs() const;

  // 计算到重置时间的延迟（毫秒）
  int calculateDelayToResetMs() const;

  // 重新从 TaskStore 加载任务，重置索引，排序
  void reloadTasks();

  // 跳过启动时已过时间的任务，返回实际起始索引
  int skipPastTasks(int startIndex) const;

  // 发射 signalNextTaskScheduled
  void emitNextTaskInfo();

  /// ==================== 核心调度逻辑 ====================

  // 执行下一个任务（第一层核心逻辑）
  void executeNextTask();

  // 等待重置时间（进入下一天周期）
  void waitForReset();

  // 开始新的周期（重置时间到达）
  void startNewCycle();

  QList<Task> tasks;
  QHash<qint32, int> mRandomOffsets; // taskId → 随机偏移秒数
  QTimer timer;
  int currentIndex = 0;
  bool running = false;
  bool skipHoliday = false;
  bool randomEnabled = false;
  int randomMaxMinutes = 5;
  QTime resetTime = QTime(0, 0); // 默认 0 点重置
};

#endif // TASKEXECUTOR_HPP
