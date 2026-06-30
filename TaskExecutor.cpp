#include "TaskExecutor.hpp"

#include "ChinaHolidayManager.hpp"
#include "Logger.hpp"
#include "TaskStore.hpp"

#include <QRandomGenerator>

#include <algorithm>

TaskExecutor::TaskExecutor(QObject *parent) : QObject(parent) {
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &TaskExecutor::onTimerTimeout);
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

void TaskExecutor::addTask(const Task &task) { tasks.append(task); }

void TaskExecutor::clearTasks() {
  tasks.clear();
  currentIndex = 0;
}

/// ==================== 私有辅助方法 ====================

void TaskExecutor::precomputeRandomOffsets() {
  mRandomOffsets.clear();
  if (!randomEnabled || randomMaxMinutes <= 0) {
    return;
  }
  for (const Task &task : tasks) {
    const int offset =
        QRandomGenerator::global()->bounded(randomMaxMinutes * 60);
    mRandomOffsets.insert(task.id, offset);
  }
  Logger::Tag("TaskExecutor")
      .dFmt("已为 %d 个任务预计算随机偏移", mRandomOffsets.size());
}

int TaskExecutor::randomOffset(qint32 taskId) const {
  return mRandomOffsets.value(taskId, 0);
}

int TaskExecutor::calculateDelayToNextMs() const {
  if (currentIndex >= tasks.size()) {
    return 0;
  }

  const QTime now = QTime::currentTime();
  const int nowSeconds = now.hour() * 3600 + now.minute() * 60 + now.second();

  const Task &task = tasks.at(currentIndex);
  const QTime scheduled = task.scheduledTime.time();
  int targetSeconds =
      scheduled.hour() * 3600 + scheduled.minute() * 60 + scheduled.second();

  // 第二层：添加随机时间波动（从缓存读取，保证一致性）
  targetSeconds += randomOffset(task.id);

  const int delaySeconds = targetSeconds - nowSeconds;
  return delaySeconds > 0 ? delaySeconds * 1000 : 0;
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
  const int nowSeconds = now.hour() * 3600 + now.minute() * 60 + now.second();

  int idx = startIndex;
  while (idx < tasks.size()) {
    const QTime taskTime = tasks.at(idx).scheduledTime.time();
    int taskSeconds =
        taskTime.hour() * 3600 + taskTime.minute() * 60 + taskTime.second();

    // 考虑随机偏移的最晚可能时间
    if (randomEnabled && randomMaxMinutes > 0) {
      taskSeconds += randomMaxMinutes * 60;
    }

    if (taskSeconds <= nowSeconds) {
      Logger::Tag("TaskExecutor")
          .dFmt("跳过已过期任务: %s",
                taskTime.toString("HH:mm:ss").toStdString().c_str());
      idx++;
    } else {
      break;
    }
  }
  return idx;
}

/// ==================== 核心调度逻辑 ====================
void TaskExecutor::onTimerTimeout() {
  if (!running) {
    return;
  }

  // 第三层：检查是否为节假日
  if (skipHoliday &&
      ChinaHolidayManager::get()->isHoliday(QDate::currentDate())) {
    Logger::Tag("TaskExecutor").d("检测到节假日，跳过今日所有任务");
    emit signalHolidaySkipped();
    waitForReset();
    return;
  }

  // 第一层：执行下一个任务
  executeNextTask();
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

  // 一次性预计算所有任务的随机偏移
  precomputeRandomOffsets();

  running = true;

  // 跳过当前时间已过的任务
  currentIndex = skipPastTasks(0);

  Logger::Tag("TaskExecutor")
      .dFmt("调度器已启动，共 %d 个任务（跳过 %d 个），重置时间=%s",
            tasks.size(), currentIndex,
            resetTime.toString("HH:mm").toStdString().c_str());

  if (currentIndex >= tasks.size()) {
    // 所有任务已过期，直接进入等待重置状态
    emit signalDayFinished();
    waitForReset();
    return;
  }

  // 启动第一个任务计时器
  const int delayMs = calculateDelayToNextMs();
  if (delayMs > 0) {
    emitNextTaskInfo();
    timer.start(delayMs);
  } else {
    // 立即执行
    onTimerTimeout();
  }
}

void TaskExecutor::stop() {
  if (!running) {
    return;
  }
  timer.stop();
  running = false;
  currentIndex = 0;
  Logger::Tag("TaskExecutor").i("调度器已停止");
}

bool TaskExecutor::isRunning() const { return running; }

void TaskExecutor::executeNextTask() {
  if (!running || currentIndex >= tasks.size()) {
    // 所有任务执行完毕
    emit signalDayFinished();
    waitForReset();
    return;
  }

  // 获取当前任务
  const Task currentTask = tasks.at(currentIndex);

  // 计算实际执行时间（计划时间 + 随机偏移，从缓存读取保证一致性）
  QDateTime actualTime = currentTask.scheduledTime;
  actualTime = actualTime.addSecs(randomOffset(currentTask.id));

  const int totalTasks = tasks.size();
  const qint32 taskId = currentTask.id;
  const int progress = currentIndex + 1; // 1-based

  // 发送任务执行信号到 MainWindow
  emit signalTaskExecuted(actualTime, taskId, progress, totalTasks);

  // 移动到下一个任务
  currentIndex++;

  if (currentIndex < tasks.size()) {
    // 还有任务，继续调度
    const int delayMs = calculateDelayToNextMs();
    emitNextTaskInfo();

    if (delayMs > 0) {
      timer.start(delayMs);
    } else {
      // 立即执行下一个
      onTimerTimeout();
    }
  } else {
    // 所有任务执行完毕
    emit signalDayFinished();
    waitForReset();
  }
}

void TaskExecutor::waitForReset() {
  const int delayMs = calculateDelayToResetMs();
  Logger::Tag("TaskExecutor")
      .dFmt("等待 %lld 秒后开始新的一天周期", delayMs / 1000);
  timer.start(delayMs);
}

void TaskExecutor::startNewCycle() {
  if (!running) {
    return;
  }

  reloadTasks();
  precomputeRandomOffsets();
  emit signalCycleReset();

  if (tasks.isEmpty()) {
    Logger::Tag("TaskExecutor").w("新周期无任务，等待下一个重置点");
    waitForReset();
    return;
  }

  // 跳过已过期的任务
  currentIndex = skipPastTasks(0);

  if (currentIndex >= tasks.size()) {
    emit signalDayFinished();
    waitForReset();
    return;
  }

  const int delayMs = calculateDelayToNextMs();
  if (delayMs > 0) {
    emitNextTaskInfo();
    timer.start(delayMs);
  } else {
    onTimerTimeout();
  }
}

void TaskExecutor::emitNextTaskInfo() {
  if (currentIndex >= tasks.size()) {
    return;
  }
  const Task &task = tasks.at(currentIndex);

  // 计算预计执行时间（含随机偏移，从缓存读取保证一致性）
  QDateTime predicted = task.scheduledTime;
  predicted = predicted.addSecs(randomOffset(task.id));

  emit signalNextTaskScheduled(currentIndex + 1, predicted, task.id);
}
