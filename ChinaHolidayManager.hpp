#ifndef CHINAHOLIDAYMANAGER_HPP
#define CHINAHOLIDAYMANAGER_HPP

#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QStringList>

class ChinaHolidayManager : public QObject {
  Q_OBJECT

public:
  static ChinaHolidayManager *get();

  void updateChinaHolidayData();

signals:
  void signalSyncSuccess(const QString &message);

  void signalSyncError(const QString &message);

private:
  explicit ChinaHolidayManager(QObject *parent = nullptr);

  QNetworkAccessManager *_networkManagerPtr;
  QStringList _urls; // 存储所有 CDN 镜像源的 URL 列表
  QString _result;   // 存储下载的节假日数据
  int _current;      // 当前正在尝试下载的 URL 索引

  void fetchHolidayData();

  void onCurlProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

  void handleHolidayData();
};

#endif // CHINAHOLIDAYMANAGER_HPP
