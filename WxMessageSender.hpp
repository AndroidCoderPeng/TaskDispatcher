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

  void sendMessage(const QString &title, const QString &content);

private:
  explicit WxMessageSender(QObject *parent = nullptr);

  QNetworkAccessManager *_networkManagerPtr;

  static constexpr const char *WX_WEBHOOK_URL =
      "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=%1";
};

#endif // WXMESSAGESENDER_HPP
