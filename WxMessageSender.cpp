#include "WxMessageSender.hpp"

#include "ConfigStore.hpp"
#include "Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QtConcurrent>

WxMessageSender *WxMessageSender::get() {
  static WxMessageSender instance;
  return &instance;
}

WxMessageSender::WxMessageSender(QObject *parent)
    : QObject(parent), _networkManagerPtr(new QNetworkAccessManager(this)) {}

void WxMessageSender::sendMessageAsync(const QString &title,
                                       const QString &content) {
  QtConcurrent::run([this, title, content]() { sendMessage(title, content); });
}

void WxMessageSender::sendImageMessageAsync(const QString &title,
                                            const QString &description,
                                            const QString &imagePath) {
  QtConcurrent::run([this, title, description, imagePath]() {
    sendImageMessage(title, description, imagePath);
  });
}

// 发送企业微信文字消息
void WxMessageSender::sendMessage(const QString &title,
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

// 发送企业微信图文消息
void WxMessageSender::sendImageMessage(const QString &title,
                                       const QString &description,
                                       const QString &imagePath) {
  // TODO
  Logger::Tag("WxMessageSender").w("sendImageMessage not implemented yet");
}