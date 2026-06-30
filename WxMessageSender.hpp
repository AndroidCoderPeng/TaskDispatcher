#ifndef WXMESSAGESENDER_HPP
#define WXMESSAGESENDER_HPP

#include <QNetworkAccessManager>
#include <QObject>

class WxMessageSender : public QObject {
  Q_OBJECT

public:
  static WxMessageSender *get();

  WxMessageSender(const WxMessageSender &) = delete;
  WxMessageSender &operator=(const WxMessageSender &) = delete;

  void sendMessageAsync(const QString &title, const QString &content);

  void sendImageMessageAsync(const QString &title, const QByteArray bytes);

private:
  explicit WxMessageSender(QObject *parent = nullptr);

  static constexpr const char *WX_WEBHOOK_URL =
      "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=%1";

  QNetworkAccessManager *_networkManagerPtr;
};

#endif // WXMESSAGESENDER_HPP
