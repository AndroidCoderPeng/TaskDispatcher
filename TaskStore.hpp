#ifndef TASKSTORE_HPP
#define TASKSTORE_HPP

#include "GlobalDefinition.hpp"

#include <QList>
#include <QSqlDatabase>

class TaskStore {
public:
  static TaskStore &get();

  TaskStore(const TaskStore &) = delete;
  TaskStore &operator=(const TaskStore &) = delete;

  /// 添加任务，返回数据库分配的 id
  qint32 add(const Task &task);

  bool update(const Task &task);

  bool remove(qint32 id);

  QList<Task> loadAll() const;

  Task loadById(qint32 id) const;

  void clear();

private:
  TaskStore();
  ~TaskStore();

  void initDatabase();
};

#endif // TASKSTORE_HPP
