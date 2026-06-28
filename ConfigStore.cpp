#include "ConfigStore.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

ConfigStore &ConfigStore::get() {
  static ConfigStore instance;
  return instance;
}

ConfigStore::ConfigStore() : _flushed(false) { readFromFile(); }

ConfigStore::~ConfigStore() {
  if (!_flushed) {
    writeToFile();
  }
}

void ConfigStore::flush() {
  writeToFile();
  _flushed = true;
}

QString ConfigStore::filePath() const {
  return QCoreApplication::applicationDirPath() + "/task_config.json";
}

void ConfigStore::readFromFile() {
  QFile file(filePath());
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  file.close();

  if (!doc.isObject()) {
    return;
  }

  const QJsonObject root = doc.object();
  for (auto it = root.begin(); it != root.end(); ++it) {
    if (it.value().isObject()) {
      _data.insert(it.key(), it.value().toObject());
    }
  }
}

void ConfigStore::writeToFile() {
  QJsonObject root;
  for (auto it = _data.begin(); it != _data.end(); ++it) {
    root.insert(it.key(), it.value());
  }

  QFile file(filePath());
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return;
  }

  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  file.close();
}

// ====== 单个键值操作 ======

void ConfigStore::save(const QString &key, const QJsonObject &value) {
  QMutexLocker locker(&_mutex);
  _data.insert(key, value);
  writeToFile();
}

QJsonObject ConfigStore::load(const QString &key) const {
  QMutexLocker locker(&_mutex);
  return _data.value(key);
}

void ConfigStore::remove(const QString &key) {
  QMutexLocker locker(&_mutex);
  if (_data.remove(key) > 0) {
    writeToFile();
  }
}

bool ConfigStore::contains(const QString &key) const {
  QMutexLocker locker(&_mutex);
  return _data.contains(key);
}

// ====== 批量操作 ======

QHash<QString, QJsonObject> ConfigStore::loadAll() const {
  QMutexLocker locker(&_mutex);
  return _data;
}

QStringList ConfigStore::keys() const {
  QMutexLocker locker(&_mutex);
  return _data.keys();
}

void ConfigStore::clear() {
  QMutexLocker locker(&_mutex);
  _data.clear();
  writeToFile();
}
