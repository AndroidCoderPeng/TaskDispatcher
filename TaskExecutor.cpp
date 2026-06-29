#include "TaskExecutor.hpp"

#include "ChinaHolidayManager.hpp"
#include "Logger.hpp"
#include "TaskStore.hpp"

#include <QRandomGenerator>

#include <algorithm>

TaskExecutor::TaskExecutor(QObject *parent) : QObject(parent) {
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
}

void TaskExecutor::setTasks(const QList<Task> &newTasks) {
  tasks = newTasks;
  currentIndex = 0;
}

void TaskExecutor::setSkipHoliday(bool skip) { skipHoliday = skip; }

void TaskExecutor::setRandomTimeConfig(bool enabled, int maxMinutes) {
  randomEnabled = enabled;
  randomMaxMinutes = maxMinutes;
}

void TaskExecutor::setResetTime(const QTime &time) { resetTime = time; }

void TaskExecutor::addTask(const Task &task) {
  Task t = task;
  t.status = TaskStatus::Pending;
  tasks.append(t);
}

void TaskExecutor::clearTasks() {
  tasks.clear();
  currentIndex = 0;
}

int TaskExecutor::calculateDelayToNextMs() const {
  if (currentIndex >= tasks.size()) {
    return 0;
  }

  const QTime now = QTime::currentTime();
  const int nowMinutes = now.hour() * 60 + now.minute();

  const QTime scheduled = tasks.at(currentIndex).scheduledTime.time();
  int targetMinutes = scheduled.hour() * 60 + scheduled.minute();

  if (randomEnabled) {
    // 使用 const_cast 在 const 方法中写入随机偏移（仅在定时器触发时调用，线程安全）
    const_cast<TaskExecutor *>(this)->currentRandomOffset =
        QRandomGenerator::global()->bounded(randomMaxMinutes + 1);
    targetMinutes += currentRandomOffset;
  }

  const int delayMinutes = targetMinutes - nowMinutes;
  return delayMinutes > 0 ? delayMinutes * 60 * 1000 : 0;
}

int TaskExecutor::calculateDelayToResetMs() const {
  const QTime now = QTime::currentTime();
  int secsToReset = now.secsTo(resetTime);
  if (secsToReset <= 0) {
    secsToReset += 24 * 3600; // 已过重置点，等待到明天
  }
  return secsToReset * 1000;
}

void TaskExecutor::reloadTasks() {
  tasks = TaskStore::get().loadAll();
  std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
    return a.scheduledTime < b.scheduledTime;
  });
  currentIndex = 0;

  Logger::Tag("TaskExecutor")
      .dFmt("新一天周期：已重新加载 %d 个任务", tasks.size());
}

int TaskExecutor::skipPastTasks(int startIndex) const {
  const QTime now = QTime::currentTime();
  const int nowMinutes = now.hour() * 60 + now.minute();

  int idx = startIndex;
  while (idx < tasks.size()) {
    const QTime taskTime = tasks.at(idx).scheduledTime.time();
    int taskMinutes = taskTime.hour() * 60 + taskTime.minute();
    if (randomEnabled) {
      taskMinutes += randomMaxMinutes; // 考虑随机偏移的最晚可能时间
    }
    if (taskMinutes <= nowMinutes) {
      Logger::Tag("TaskExecutor")
          .dFmt("跳过已过期任务: %s",
                taskTime.toString("HH:mm").toStdString().c_str());
      idx++;
    } else {
      break;
    }
  }
  return idx;
}

void TaskExecutor::start() {
  if (running) {
    return;
  }

  // 加载任务
  tasks = TaskStore::get().loadAll();
  if (tasks.isEmpty()) {
    Logger::Tag("TaskExecutor").w("没有任务，无法启动");
    return;
  }

  std::sort(tasks.begin(), tasks.end(), [](const Task &a, const Task &b) {
    return a.scheduledTime < b.scheduledTime;
  });

  running = true;

  // 跳过当前时间已过的任务，直接从下一个未到期任务开始
  currentIndex = skipPastTasks(0);

  Logger::Tag("TaskExecutor")
      .dFmt("调度器已启动，共 %d 个任务（跳过 %d 个），重置时间=%s",
            tasks.size(), currentIndex,
            resetTime.toString("HH:mm").toStdString().c_str());

  if (currentIndex >= tasks.size()) {
    // 所有任务已过期，直接进入等待重置状态
    emit signalDayFinished();
    const int delayMs = calculateDelayToResetMs();
    Logger::Tag("TaskExecutor")
        .dFmt("所有任务已过期，%lld 秒后等待重置", delayMs / 1000);
    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    timer.start(delayMs);
    return;
  }

  const int delayMs = calculateDelayToNextMs();
  if (delayMs > 0) {
    emitNextTaskInfo(); // 预显示第一个待执行任务
    timer.start(delayMs);
  } else {
    executeNextTask();
  }
}

void TaskExecutor::stop() {
  if (!running) {
    return;
  }
  timer.stop();
  running = false;
  currentIndex = 0;
  Logger::Tag("TaskExecutor").d("调度器已停止");
}

bool TaskExecutor::isRunning() const { return running; }

void TaskExecutor::executeNextTask() {
  if (!running) {
    return;
  }

  // 当天所有任务已执行完毕 → 等待任务重置点，开启下一天
  if (currentIndex >= tasks.size()) {
    emit signalDayFinished();

    const int delayMs = calculateDelayToResetMs();
    Logger::Tag("TaskExecutor")
        .dFmt("当日任务全部完成，%lld 秒后重置", delayMs / 1000);

    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    timer.start(delayMs);
    return;
  }

  // 每个节点执行前检查节假日
  if (skipHoliday &&
      ChinaHolidayManager::get()->isHoliday(QDate::currentDate())) {
    emit signalHolidaySkipped();

    // 当日跳过，等待重置点
    const int delayMs = calculateDelayToResetMs();
    Logger::Tag("TaskExecutor")
        .dFmt("节假日：当日任务已跳过，%lld 秒后重置", delayMs / 1000);

    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    timer.start(delayMs);
    return;
  }

  // 执行当前任务：计算实际执行时间（计划时间 + 随机偏移）
  const Task currentTask = tasks.at(currentIndex);
  const QDateTime actualTime =
      currentTask.scheduledTime.addSecs(currentRandomOffset * 60);
  const int totalTasks = tasks.size();
  const qint32 taskId = currentTask.id;
  const int progress = currentIndex + 1; // 1-based

  emit signalTaskExecuted(actualTime, taskId, progress, totalTasks);

  currentIndex++;
  if (currentIndex < tasks.size()) {
    // 恢复到正常连接
    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);

    const int delayMs = calculateDelayToNextMs();
    emitNextTaskInfo(); // 预显示下一个待执行任务

    if (delayMs > 0) {
      timer.start(delayMs);
    } else {
      executeNextTask();
    }
  } else {
    // 已经是最后一个任务，进入"等待重置"状态
    emit signalDayFinished();

    const int delayMs = calculateDelayToResetMs();
    Logger::Tag("TaskExecutor")
        .dFmt("当日任务全部完成，%lld 秒后重置", delayMs / 1000);

    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    timer.start(delayMs);
  }
}

void TaskExecutor::emitNextTaskInfo() {
  if (currentIndex >= tasks.size()) {
    return;
  }
  const Task &task = tasks.at(currentIndex);
  const QDateTime predicted =
      task.scheduledTime.addSecs(currentRandomOffset * 60);
  emit signalNextTaskScheduled(currentIndex + 1, predicted, task.id);
}

void TaskExecutor::startNewCycle() {
  if (!running) {
    return;
  }

  // 恢复信号连接到正常执行
  disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
  connect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);

  reloadTasks();
  emit signalCycleReset();

  if (tasks.isEmpty()) {
    // 如果用户删除了所有任务，再等一天
    Logger::Tag("TaskExecutor").w("新周期无任务，等待下一个重置点");
    const int delayMs = calculateDelayToResetMs();
    timer.start(delayMs);
    // 重新连接到 startNewCycle
    disconnect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
    connect(&timer, &QTimer::timeout, this, &TaskExecutor::startNewCycle);
    return;
  }

  const int delayMs = calculateDelayToNextMs();
  if (delayMs > 0) {
    emitNextTaskInfo(); // 预显示第一个待执行任务
    timer.start(delayMs);
  } else {
    executeNextTask();
  }
}
