#include "ChinaHolidayManager.hpp"
#include "ConfigStore.hpp"
#include "Logger.hpp"

#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

// CDN 镜像源列表
static const QStringList cdnUrls = {
    "https://cdn.jsdelivr.net/npm/chinese-days/dist/years/%1.json",
    "https://fastly.jsdelivr.net/npm/chinese-days/dist/years/%1.json",
    "https://registry.npmmirror.com/chinese-days/latest/files/dist/years/"
    "%1.json"};

ChinaHolidayManager *ChinaHolidayManager::get() {
  static ChinaHolidayManager instance;
  return &instance;
}

ChinaHolidayManager::ChinaHolidayManager(QObject *parent)
    : QObject(parent), _networkManagerPtr(new QNetworkAccessManager(this)) {}

void ChinaHolidayManager::updateChinaHolidayData() {
  // 先尝试从缓存加载
  tryLoadFromCache();

  // 缓存命中则直接返回
  if (!_holidayDates.isEmpty()) {
    return;
  }

  // 缓存未命中，走网络下载
  _urls.clear();
  _result.clear();

  const int currentYear = QDate::currentDate().year();

  for (const QString &tpl : cdnUrls) {
    const QString url = tpl.arg(currentYear);
    _urls.append(url);
  }

  fetchHolidayData();
}

void ChinaHolidayManager::fetchHolidayData() {
  // 如果本年度已下载完成，解析数据并结束
  if (!_result.isEmpty()) {
    QJsonDocument doc = QJsonDocument::fromJson(_result.toUtf8());
    if (doc.isNull()) {
      emit signalSyncError("节假日数据解析失败：JSON 无效");
      return;
    }

    handleHolidayData();
    return;
  }

  if (_urls.isEmpty()) {
    emit signalSyncError("节假日数据下载失败：所有镜像源均不可用");
    return;
  }

  QString url = _urls.takeFirst();
  QUrl q_url = QUrl(url);
  QNetworkRequest request(q_url);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);
  QNetworkReply *reply = _networkManagerPtr->get(request);

  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      Logger::Tag("ChinaHolidayManager")
          .dFmt("Network error: %s",
                reply->errorString().toStdString().c_str());
      fetchHolidayData();
      return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
      Logger::Tag("ChinaHolidayManager").i("Invalid JSON from network reply");
      fetchHolidayData();
      return;
    }

    _result = QString::fromUtf8(data);
    fetchHolidayData();
  });
}

void ChinaHolidayManager::handleHolidayData() {
  QJsonDocument doc = QJsonDocument::fromJson(_result.toUtf8());
  if (doc.isNull() || !doc.isObject()) {
    emit signalSyncError("节假日数据解析失败：JSON 无效");
    return;
  }

  QJsonObject root = doc.object();

  parseJsonToMemory(root);
  QJsonObject cache;
  cache["year"] = QDate::currentDate().year();
  cache["holidays"] = root.value("holidays");
  cache["workdays"] = root.value("workdays");
  ConfigStore::get().save("holidayConfig", cache);

  emit signalSyncSuccess("节假日数据同步完成");
}

void ChinaHolidayManager::parseJsonToMemory(const QJsonObject &root) {
  _holidayDates.clear();
  QJsonObject holidays = root.value("holidays").toObject();
  for (auto it = holidays.begin(); it != holidays.end(); ++it) {
    QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
    if (date.isValid()) {
      _holidayDates.insert(date);
    }
  }

  _workdayDates.clear();
  QJsonObject workdays = root.value("workdays").toObject();
  for (auto it = workdays.begin(); it != workdays.end(); ++it) {
    QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
    if (date.isValid()) {
      _workdayDates.insert(date);
    }
  }

  Logger::Tag("ChinaHolidayManager")
      .dFmt("Parsed %d holidays, %d workdays", _holidayDates.size(),
            _workdayDates.size());
}

void ChinaHolidayManager::tryLoadFromCache() {
  QJsonObject cache = ConfigStore::get().load("holidayConfig");
  if (cache.isEmpty()) {
    return;
  }

  int cachedYear = cache.value("year").toInt();
  int currentYear = QDate::currentDate().year();
  if (cachedYear != currentYear) {
    Logger::Tag("ChinaHolidayManager")
        .dFmt("Cache year %d != current %d, will re-fetch", cachedYear,
              currentYear);
    return;
  }

  parseJsonToMemory(cache);
  Logger::Tag("ChinaHolidayManager").i("Loaded holidays from cache");
}

bool ChinaHolidayManager::isHoliday(const QDate &date) const {
  return _holidayDates.contains(date);
}

bool ChinaHolidayManager::isWorkday(const QDate &date) const {
  return _workdayDates.contains(date);
}
