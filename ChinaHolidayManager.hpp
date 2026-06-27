#ifndef CHINAHOLIDAYMANAGER_HPP
#define CHINAHOLIDAYMANAGER_HPP

#include <QDate>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QStringList>

class ChinaHolidayManager : public QObject {
  Q_OBJECT

public:
  static ChinaHolidayManager *get();

  void updateChinaHolidayData();

  // 给定日期是否是法定节假日（含调休放假，不含周末）
  bool isHoliday(const QDate &date) const;

  // 给定日期是否是调休补班日
  bool isWorkday(const QDate &date) const;

signals:
  void signalSyncSuccess(const QString &message);

  void signalSyncError(const QString &message);

private:
  explicit ChinaHolidayManager(QObject *parent = nullptr);

  QNetworkAccessManager *_networkManagerPtr;
  QStringList _urls;         // 存储所有 CDN 镜像源的 URL 列表
  QString _result;           // 存储下载的节假日数据
  int _current;              // 当前正在尝试下载的 URL 索引
  QSet<QDate> _holidayDates; // 节假日日期集合（含调休放假）
  QSet<QDate> _workdayDates; // 调休补班日期集合

  void tryLoadFromCache();

  void fetchHolidayData();

  void onCurlProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

  void handleHolidayData();

  void parseJsonToMemory(const QJsonObject &root);
};

#endif // CHINAHOLIDAYMANAGER_HPP
