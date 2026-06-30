#include "WxMessageSender.hpp"

#include "ConfigStore.hpp"
#include "Logger.hpp"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

WxMessageSender *WxMessageSender::get() {
  static WxMessageSender instance;
  return &instance;
}

WxMessageSender::WxMessageSender(QObject *parent)
    : QObject(parent), _networkManagerPtr(new QNetworkAccessManager(this)) {}

// 发送企业微信文字消息
void WxMessageSender::sendMessageAsync(const QString &title,
                                       const QString &content) {
  QJsonObject cfg = ConfigStore::get().load("wxConfig");
  if (cfg.isEmpty()) {
    Logger::Tag("WxMessageSender").e("WxConfig not found");
    return;
  }

  QString wxKey = cfg.value("wxKey").toString();
  if (wxKey.isEmpty()) {
    Logger::Tag("WxMessageSender").e("WxKey is empty");
    return;
  }

  QString url = QString(WX_WEBHOOK_URL).arg(wxKey);

  QJsonObject textObj;
  textObj["content"] = title + "\n" + content;

  QJsonObject bodyObj;
  bodyObj["msgtype"] = "text";
  bodyObj["text"] = textObj;

  QByteArray jsonData = QJsonDocument(bodyObj).toJson(QJsonDocument::Compact);

  Logger::Tag("WxMessageSender")
      .dFmt("Sending wx message to: %s", url.toStdString().c_str());

  QUrl q_url = QUrl(url);
  QNetworkRequest request(q_url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = _networkManagerPtr->post(request, jsonData);

  connect(reply, &QNetworkReply::finished, this, [reply]() {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      Logger::Tag("WxMessageSender")
          .dFmt("Network error: %s",
                reply->errorString().toStdString().c_str());
      return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject resp = doc.object();

    int errcode = resp.value("errcode").toInt(-1);
    if (errcode != 0) {
      QString errmsg = resp.value("errmsg").toString();
      Logger::Tag("WxMessageSender")
          .eFmt("Wx API error: %d - %s", errcode, errmsg.toStdString().c_str());
      return;
    }

    Logger::Tag("WxMessageSender").i("Wx message sent successfully");
  });
}

// 发送企业微信图片消息
void WxMessageSender::sendImageMessageAsync(const QString &title,
                                            const QByteArray bytes) {
  QJsonObject cfg = ConfigStore::get().load("wxConfig");
  if (cfg.isEmpty()) {
    Logger::Tag("WxMessageSender").e("WxConfig not found");
    return;
  }

  QString wxKey = cfg.value("wxKey").toString();
  if (wxKey.isEmpty()) {
    Logger::Tag("WxMessageSender").e("WxKey is empty");
    return;
  }

  if (bytes.isEmpty()) {
    Logger::Tag("WxMessageSender").e("Image data is empty");
    return;
  }

  // 企业微信图片消息限制：base64 编码前不超过 2MB
  constexpr int kMaxImageSize = 2 * 1024 * 1024;
  if (bytes.size() > kMaxImageSize) {
    Logger::Tag("WxMessageSender")
        .eFmt("Image too large: %d bytes (max %d)", bytes.size(),
              kMaxImageSize);
    return;
  }

  QString url = QString(WX_WEBHOOK_URL).arg(wxKey);

  // 计算 base64 和 MD5
  const QString base64 = QString::fromUtf8(bytes.toBase64());
  const QString md5 = QString::fromUtf8(
      QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex());

  QJsonObject imageObj;
  imageObj["base64"] = base64;
  imageObj["md5"] = md5;

  QJsonObject bodyObj;
  bodyObj["msgtype"] = "image";
  bodyObj["image"] = imageObj;

  QByteArray jsonData = QJsonDocument(bodyObj).toJson(QJsonDocument::Compact);

  Logger::Tag("WxMessageSender")
      .dFmt("Sending wx image message to: %s (%d bytes)",
            url.toStdString().c_str(), bytes.size());

  QUrl qUrl(url);
  QNetworkRequest request(qUrl);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QNetworkReply *reply = _networkManagerPtr->post(request, jsonData);

  connect(reply, &QNetworkReply::finished, this, [reply, title]() {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
      Logger::Tag("WxMessageSender")
          .eFmt("Network error: %s",
                reply->errorString().toStdString().c_str());
      return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject resp = doc.object();

    int errcode = resp.value("errcode").toInt(-1);
    if (errcode != 0) {
      QString errmsg = resp.value("errmsg").toString();
      Logger::Tag("WxMessageSender")
          .eFmt("Wx API error: %d - %s", errcode, errmsg.toStdString().c_str());
      return;
    }

    Logger::Tag("WxMessageSender").i("Wx image sent successfully");
  });
}