#ifndef MAILSENDER_HPP
#define MAILSENDER_HPP

#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>

#include "GlobalDefinition.hpp"

class MailSender : public QObject {
  Q_OBJECT

public:
  static MailSender *get();

  MailSender(const MailSender &) = delete;
  MailSender &operator=(const MailSender &) = delete;

  void sendEmail(const QString &subject, const QString &body);

  void sendAttachmentEmail(const QString &subject, const QString &body,
                           const QByteArray bytes);

private slots:
  void onConnected();
  void onEncrypted();
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError error);
  void onTimeout();

private:
  explicit MailSender(QObject *parent = nullptr);
  ~MailSender() override;

  bool loadEmailConfig();

  QString buildMailContent(const QString &subject, const QString &body) const;

  QString buildAttachmentMailContent(const QString &subject,
                                     const QString &body,
                                     const QByteArray &attachmentData,
                                     const QString &fileName) const;

  // 发送 SMTP 命令并等待响应码
  void sendCommand(const QString &cmd);

  // 检查响应行是否为最终行（SMTP 多行响应以 code+空格开头为结束行）
  static bool isFinalLine(const QString &line, int expectedCode);

  // 处理一条完整的 SMTP 响应
  void handleSmtpResponse(int code);

  // 发送失败清理
  void fail(const QString &reason);

  QSslSocket *socketPtr = nullptr;
  QTimer *timeoutTimerPtr = nullptr;
  EmailConfig config;
  QString pendingSubject;
  QString pendingBody;
  QByteArray pendingAttachmentData;
  QString pendingAttachmentFileName;
  int smtpStep = 0;
  int expectedCode = 0;
  QString multiLineBuffer;

  static constexpr int SMTP_TIMEOUT_MS = 60000;
  static constexpr const char *SMTP_HOST = "smtp.qq.com";
  static constexpr int SMTP_PORT = 465;
};

#endif // MAILSENDER_HPP
