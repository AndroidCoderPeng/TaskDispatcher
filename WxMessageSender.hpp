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

  void sendImageMessageAsync(const QString &title, const QString &description,
                             const QString &imagePath);

private:
  explicit WxMessageSender(QObject *parent = nullptr);

  QNetworkAccessManager *_networkManagerPtr;

  static constexpr const char *WX_WEBHOOK_URL =
      "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=%1";

  void sendMessage(const QString &title, const QString &content);

  void sendImageMessage(const QString &title, const QString &description,
                        const QString &imagePath);
};

#endif // WXMESSAGESENDER_HPP
