#include "TaskStore.hpp"

#include <QCoreApplication>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

TaskStore &TaskStore::get() {
  static TaskStore instance;
  return instance;
}

TaskStore::TaskStore() { initDatabase(); }

TaskStore::~TaskStore() {
  if (QSqlDatabase::database("task_conn").isOpen()) {
    QSqlDatabase::database("task_conn").close();
  }
}

void TaskStore::initDatabase() {
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "task_conn");
  db.setDatabaseName(QCoreApplication::applicationDirPath() + "/tasks.db");

  if (!db.open()) {
    qWarning("TaskStore: Failed to open database: %s",
             db.lastError().text().toStdString().c_str());
    return;
  }

  QSqlQuery query(db);
  query.exec("CREATE TABLE IF NOT EXISTS tasks ("
             "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
             "  scheduled_time TEXT NOT NULL"
             ")");
}

// ====== 增删改查 ======

qint32 TaskStore::add(const Task &task) {
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.prepare("INSERT INTO tasks (scheduled_time) VALUES (:scheduled)");
  query.bindValue(":scheduled", task.scheduledTime.toString("HH:mm:ss"));

  if (!query.exec()) {
    qWarning("TaskStore: Failed to add task: %s",
             query.lastError().text().toStdString().c_str());
    return -1;
  }
  return query.lastInsertId().toInt();
}

bool TaskStore::update(const Task &task) {
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.prepare("UPDATE tasks SET scheduled_time = :scheduled WHERE id = :id");
  query.bindValue(":scheduled", task.scheduledTime.toString("HH:mm:ss"));
  query.bindValue(":id", task.id);

  if (!query.exec()) {
    qWarning("TaskStore: Failed to update task: %s",
             query.lastError().text().toStdString().c_str());
    return false;
  }
  return query.numRowsAffected() > 0;
}

bool TaskStore::remove(qint32 id) {
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.prepare("DELETE FROM tasks WHERE id = :id");
  query.bindValue(":id", id);

  if (!query.exec()) {
    qWarning("TaskStore: Failed to remove task: %s",
             query.lastError().text().toStdString().c_str());
    return false;
  }
  return query.numRowsAffected() > 0;
}

QList<Task> TaskStore::loadAll() const {
  QList<Task> result;
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.exec("SELECT id, scheduled_time FROM tasks");

  while (query.next()) {
    Task task;
    task.id = query.value(0).toInt();
    task.scheduledTime =
        QDateTime::fromString(query.value(1).toString(), "HH:mm:ss");
    result.append(task);
  }
  return result;
}

Task TaskStore::loadById(qint32 id) const {
  Task task;
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.prepare("SELECT id, scheduled_time FROM tasks WHERE id = :id");
  query.bindValue(":id", id);

  if (query.exec() && query.next()) {
    task.id = query.value(0).toInt();
    task.scheduledTime =
        QDateTime::fromString(query.value(1).toString(), "HH:mm:ss");
  }
  return task;
}

void TaskStore::clear() {
  QSqlDatabase db = QSqlDatabase::database("task_conn");
  QSqlQuery query(db);
  query.exec("DELETE FROM tasks");
}
