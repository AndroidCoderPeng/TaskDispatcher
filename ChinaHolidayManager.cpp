#include "ChinaHolidayManager.hpp"
#include "Logger.hpp"

#include <QDate>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
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
  // 这里可以根据实际需求解析 _result 中的节假日数据
  // 例如，将其存储到本地文件或数据库中
  Logger::Tag("ChinaHolidayManager").d(_result.toStdString().c_str());
  emit signalSyncSuccess("节假日数据同步完成");
}
