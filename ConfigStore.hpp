#ifndef CONFIGSTORE_HPP
#define CONFIGSTORE_HPP

#include <QHash>
#include <QJsonObject>
#include <QMutex>
#include <QString>

class ConfigStore {
public:
  static ConfigStore &get();

  // 禁止拷贝和移动
  ConfigStore(const ConfigStore &) = delete;
  ConfigStore &operator=(const ConfigStore &) = delete;

  // ====== 单个键值操作 ======

  void save(const QString &key, const QJsonObject &value);

  QJsonObject load(const QString &key) const;

  void remove(const QString &key);

  bool contains(const QString &key) const;

  // ====== 批量操作 ======

  QHash<QString, QJsonObject> loadAll() const;

  QStringList keys() const;

  void clear();

  /// 在 Application 析构前主动刷新数据到磁盘
  void flush();

private:
  ConfigStore();
  ~ConfigStore();

  QString filePath() const;

  void readFromFile();

  void writeToFile();

  QHash<QString, QJsonObject> _data;
  mutable QMutex _mutex;
  bool _flushed = false;
};

#endif // CONFIGSTORE_HPP
