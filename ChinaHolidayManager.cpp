#include "ChinaHolidayManager.hpp"
#include "Logger.hpp"

#include <QDate>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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
  _urls.clear();
  _result.clear();

  const int currentYear = QDate::currentDate().year();

  // 构建所有 URL：本年度 × 每个 CDN 源（用于容错）
  for (const QString &tpl : cdnUrls) {
    const QString url = tpl.arg(currentYear);
    _urls.append(url);
  }

  _current = 0;

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

  // 用系统 curl 下载，绕过 Qt 的 SSL 依赖
  QProcess *process = new QProcess(this);
  process->start("curl", {"-sSL", url});
  connect(process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &ChinaHolidayManager::onCurlProcessFinished);
}

void ChinaHolidayManager::onCurlProcessFinished(
    int exitCode, QProcess::ExitStatus exitStatus) {
  Q_UNUSED(exitStatus);

  QProcess *process = qobject_cast<QProcess *>(sender());
  if (!process) {
    return;
  }
  process->deleteLater();

  if (exitCode != 0) {
    Logger::Tag("ChinaHolidayManager")
        .dFmt("curl failed: %s",
              process->readAllStandardError().toStdString().c_str());
    fetchHolidayData();
    return;
  }

  QByteArray data = process->readAllStandardOutput();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull()) {
    Logger::Tag("ChinaHolidayManager").i("Invalid JSON from curl output");
    fetchHolidayData();
    return;
  }

  _result = QString::fromUtf8(data);
  fetchHolidayData();
}

void ChinaHolidayManager::handleHolidayData() {
  QJsonDocument doc = QJsonDocument::fromJson(_result.toUtf8());
  if (doc.isNull() || !doc.isObject()) {
    emit signalSyncError("节假日数据解析失败：JSON 无效");
    return;
  }

  QJsonObject root = doc.object();

  // 解析节假日日期
  _holidayDates.clear();
  QJsonObject holidays = root.value("holidays").toObject();
  for (auto it = holidays.begin(); it != holidays.end(); ++it) {
    QDate date = QDate::fromString(it.key(), "yyyy-MM-dd");
    if (date.isValid()) {
      _holidayDates.insert(date);
    }
  }

  // 解析调休补班日期
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

  emit signalSyncSuccess("节假日数据同步完成");
}

bool ChinaHolidayManager::isHoliday(const QDate &date) const {
  // 只有周末不算节假日（因为周末不上班属于正常休息），
  // 这里只返回法定节假日 + 调休放假
  return _holidayDates.contains(date);
}

bool ChinaHolidayManager::isWorkday(const QDate &date) const {
  // 调休补班日：本应是周末但被调整为工作日
  return _workdayDates.contains(date);
}
