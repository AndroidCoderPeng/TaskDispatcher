#include "TaskExecutor.hpp"

TaskExecutor::TaskExecutor(QObject *parent)
    : QObject(parent), currentIndex(0), running(false) {
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, this, &TaskExecutor::executeNextTask);
}

void TaskExecutor::setTasks(const QList<Task> &newTasks) {
  if (running) {
    stop();
  }
  tasks = newTasks;
  currentIndex = 0;
}

void TaskExecutor::addTask(const QString &time) {
  Task task;
  // task.scheduledTime = time;
  task.status = TaskStatus::Pending;
  tasks.append(task);
}

void TaskExecutor::clearTasks() {
  if (running) {
    stop();
  }
  tasks.clear();
  currentIndex = 0;
}

void TaskExecutor::start() {
  if (running || tasks.isEmpty()) {
    return;
  }

  running = true;
  currentIndex = 0;
  executeNextTask(); // 第一个任务立即执行
}

void TaskExecutor::stop() {
  if (!running) {
    return;
  }

  timer.stop();
  running = false;
  currentIndex = 0;
  emit signalTaskFinished();
}

bool TaskExecutor::isRunning() const { return running; }

void TaskExecutor::executeNextTask() {
  if (!running) {
    return;
  }

  if (currentIndex >= tasks.size()) {
    timer.stop();
    running = false;
    emit signalTaskFinished();
    return;
  }

  const Task currentTask = tasks.at(currentIndex);
  emit signalTaskExecuted(currentTask.scheduledTime);

  currentIndex++;
  if (currentIndex < tasks.size()) {
    // const int nextDelayMs = currentTask.interval > 0 ? currentTask.interval :
    // 1; timer.start(nextDelayMs);
  } else {
    running = false;
    emit signalTaskFinished();
  }
}