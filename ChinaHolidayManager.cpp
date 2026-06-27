#include "ChinaHolidayManager.hpp"
#include "Logger.hpp"

#include <QDate>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

// CDN 镜像源列表
static const QStringList cdnUrls = {
    "http://cdn.jsdelivr.net/npm/chinese-days/dist/years/%1.json",
    "http://fastly.jsdelivr.net/npm/chinese-days/dist/years/%1.json",
};

ChinaHolidayManager *ChinaHolidayManager::get() {
  static ChinaHolidayManager instance;
  return &instance;
}

ChinaHolidayManager::ChinaHolidayManager(QObject *parent)
    : QObject(parent), _networkManagerPtr(new QNetworkAccessManager(this)) {}

void ChinaHolidayManager::updateChinaHolidayData() {
  _urls.clear();
  _result.clear();

  const int currentYear = QDate::currentDate().year();

  // 构建所有 URL：本年度 × 每个 CDN 源（用于容错）
  for (const QString &tpl : cdnUrls) {
    const QString url = tpl.arg(currentYear);
    _urls.append(url);
  }

  _current = 0;
  _total = _urls.size();

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

    // 解析 JSON 并发送具体数据信号
    handleHolidayData();
    return;
  }

  if (_urls.isEmpty()) {
    emit signalSyncError("节假日数据下载失败：所有镜像源均不可用");
    return;
  }

  QString url = _urls.takeFirst();
  _current++;

  emit signalSyncChinaHoliday(_current, _total,
                              QString("正在下载: %1").arg(url));

  QUrl q_url(url);
  QNetworkRequest request(q_url);
  QNetworkReply *reply = _networkManagerPtr->get(request);
  connect(reply, &QNetworkReply::finished, this,
          &ChinaHolidayManager::onReplyFinished);
}

void ChinaHolidayManager::onReplyFinished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (!reply) {
    fetchHolidayData();
    return;
  }
  reply->deleteLater();

  QString url = reply->url().toString();

  if (reply->error() != QNetworkReply::NoError) {
    Logger::Tag("ChinaHolidayManager")
        .dFmt("Download failed: %s, error: %s", url.toStdString().c_str(),
              reply->errorString().toStdString().c_str());
    // 尝试下一个 CDN 源
    fetchHolidayData();
    return;
  }

  QByteArray data = reply->readAll();
  Logger::Tag("ChinaHolidayManager")
      .dFmt("Raw response (first 500 chars): %s",
            data.left(500).toStdString().c_str());
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (doc.isNull()) {
    Logger::Tag("ChinaHolidayManager")
        .dFmt("Invalid JSON from: %s", url.toStdString().c_str());
    // 尝试下一个 CDN 源
    fetchHolidayData();
    return;
  }

  _result = QString::fromUtf8(data);

  // 最后调用一次fetchHolidayData是为了触发handleHolidayData解析数据
  fetchHolidayData();
}

void ChinaHolidayManager::handleHolidayData() {
  // 这里可以根据实际需求解析 _result 中的节假日数据
  // 例如，将其存储到本地文件或数据库中
  emit signalSyncChinaHoliday(_current, _total, "节假日数据同步完成");
}
