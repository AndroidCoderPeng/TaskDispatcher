#include "MailSender.hpp"

#include "ConfigStore.hpp"
#include "Logger.hpp"

#include <QCoreApplication>
#include <QDateTime>

MailSender *MailSender::get() {
  static MailSender instance;
  return &instance;
}

MailSender::MailSender(QObject *parent) : QObject(parent) {
  socketPtr = new QSslSocket(this);

  connect(socketPtr, &QSslSocket::connected, this, &MailSender::onConnected);
  connect(socketPtr, &QSslSocket::encrypted, this, &MailSender::onEncrypted);
  connect(socketPtr, &QSslSocket::readyRead, this, &MailSender::onReadyRead);
  connect(socketPtr,
          QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
          this, &MailSender::onErrorOccurred);

  timeoutTimerPtr = new QTimer(this);
  timeoutTimerPtr->setSingleShot(true);
  connect(timeoutTimerPtr, &QTimer::timeout, this, &MailSender::onTimeout);
}

MailSender::~MailSender() {
  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    socketPtr->disconnectFromHost();
  }
}

bool MailSender::loadEmailConfig() {
  QJsonObject obj = ConfigStore::get().load("emailConfig");
  if (obj.isEmpty()) {
    return false;
  }
  config.emailTitle = obj["emailTitle"].toString();
  config.senderEmail = obj["senderEmail"].toString();
  config.authCode = obj["authCode"].toString();
  config.receiverEmail = obj["receiverEmail"].toString();

  if (config.senderEmail.isEmpty() || config.authCode.isEmpty() ||
      config.receiverEmail.isEmpty()) {
    return false;
  }
  return true;
}

void MailSender::sendEmail(const QString &subject, const QString &body) {
  if (!loadEmailConfig()) {
    Logger::Tag("MailSender").e("Email config not found or incomplete");
    emit signalSendError("邮箱配置不完整，请先在配置邮箱信息");
    return;
  }

  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    Logger::Tag("MailSender").w("Socket is busy, aborting previous connection");
    socketPtr->abort();
  }

  pendingSubject = subject;
  pendingBody = body;
  smtpStep = 0;
  multiLineBuffer.clear();

  Logger::Tag("MailSender").dFmt("Connecting to %s:%d", SMTP_HOST, SMTP_PORT);
  socketPtr->connectToHostEncrypted(SMTP_HOST, SMTP_PORT);
  timeoutTimerPtr->start(SMTP_TIMEOUT_MS);
}

void MailSender::onConnected() {
  Logger::Tag("MailSender").d("TCP connected, waiting for SSL handshake...");
}

void MailSender::onEncrypted() {
  Logger::Tag("MailSender").d("SSL handshake completed");
}

void MailSender::onReadyRead() {
  timeoutTimerPtr->start(SMTP_TIMEOUT_MS);

  while (socketPtr->canReadLine()) {
    QString line = QString::fromUtf8(socketPtr->readLine()).trimmed();
    Logger::Tag("MailSender").dFmt("Received: %s", line.toStdString().c_str());

    int code = line.left(3).toInt();
    if (code == 0) {
      continue; // 无法解析响应码的行，跳过
    }

    // SMTP 多行响应：4xx/5xx 开头表示错误，直接失败
    if (code >= 400) {
      fail("服务器返回错误: " + line);
      return;
    }

    // 检查是否为多行响应的最终行（code + 空格开头）
    if (!isFinalLine(line, expectedCode)) {
      continue; // 中间行，跳过
    }

    // 最终行到达，处理响应
    handleSmtpResponse(code);
  }
}

bool MailSender::isFinalLine(const QString &line, int expectedCode) {
  if (line.length() < 4)
    return true;
  // SMTP 多行响应：中间行是 "250-xxx"，最终行是 "250 xxx"
  QChar sep = line.at(3);
  return sep == ' ';
}

void MailSender::handleSmtpResponse(int code) {
  switch (smtpStep) {
  case 0: // 服务器问候 (220)
    smtpStep = 1;
    expectedCode = 250;
    sendCommand("EHLO TaskDispatcher");
    break;

  case 1: // EHLO 响应完毕 (250)
    smtpStep = 2;
    expectedCode = 334;
    sendCommand("AUTH LOGIN");
    break;

  case 2: // AUTH LOGIN 响应 (334)
    smtpStep = 3;
    expectedCode = 334;
    sendCommand(QString::fromUtf8(config.senderEmail.toUtf8().toBase64()));
    break;

  case 3: // 用户名响应 (334)
    smtpStep = 4;
    expectedCode = 235;
    sendCommand(QString::fromUtf8(config.authCode.toUtf8().toBase64()));
    break;

  case 4: // 认证成功 (235)
    smtpStep = 5;
    expectedCode = 250;
    sendCommand(QString("MAIL FROM:<%1>").arg(config.senderEmail.trimmed()));
    break;

  case 5: // MAIL FROM 响应 (250)
    smtpStep = 6;
    expectedCode = 250;
    sendCommand(QString("RCPT TO:<%1>").arg(config.receiverEmail.trimmed()));
    break;

  case 6: // RCPT TO 响应 (250)
    smtpStep = 7;
    expectedCode = 354;
    sendCommand("DATA");
    break;

  case 7: // DATA 就绪 (354)，发送邮件内容
  {
    smtpStep = 8;
    expectedCode = 250;
    QString mailContent = buildMailContent(pendingSubject, pendingBody);
    Logger::Tag("MailSender").d("Sending mail content...");
    const QStringList lines = mailContent.split("\r\n");
    for (const QString &l : lines) {
      // SMTP 透明传输：行首的 . 需要转义为 ..
      if (l.startsWith('.')) {
        socketPtr->write(("." + l).toUtf8() + "\r\n");
      } else {
        socketPtr->write(l.toUtf8() + "\r\n");
      }
    }
    socketPtr->write("\r\n.\r\n");
    socketPtr->flush();
  } break;

  case 8: // 邮件内容已接收 (250)
    smtpStep = 9;
    expectedCode = 221;
    sendCommand("QUIT");
    break;

  case 9: // QUIT 响应 (221)
    timeoutTimerPtr->stop();
    socketPtr->disconnectFromHost();
    Logger::Tag("MailSender").i("Email sent successfully!");
    emit signalSendSuccess("邮件发送成功！");
    break;

  default:
    break;
  }
}

void MailSender::sendCommand(const QString &cmd) {
  Logger::Tag("MailSender").dFmt("Sending: %s", cmd.toStdString().c_str());
  socketPtr->write(cmd.toUtf8() + "\r\n");
  socketPtr->flush();
}

void MailSender::fail(const QString &reason) {
  timeoutTimerPtr->stop();
  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    socketPtr->disconnectFromHost();
  }
  Logger::Tag("MailSender")
      .eFmt("Send failed: %s", reason.toStdString().c_str());
  emit signalSendError(reason);
}

void MailSender::onErrorOccurred(QAbstractSocket::SocketError error) {
  Q_UNUSED(error)
  fail("邮件发送失败: " + socketPtr->errorString());
}

void MailSender::onTimeout() { fail("邮件发送超时，请检查网络连接或邮箱配置"); }

QString MailSender::buildMailContent(const QString &subject,
                                     const QString &body) const {
  QString from = config.senderEmail.trimmed();
  QString to = config.receiverEmail.trimmed();
  QString boundary = "TaskDispatcherBoundary_" +
                     QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");

  QString content;
  content += "From: " + from + "\r\n";
  content += "To: " + to + "\r\n";
  content += "Subject: =?UTF-8?B?" + subject.toUtf8().toBase64() + "?=\r\n";
  content += "MIME-Version: 1.0\r\n";
  content +=
      "Content-Type: multipart/alternative; boundary=\"" + boundary + "\"\r\n";
  content += "\r\n";
  content += "--" + boundary + "\r\n";
  content += "Content-Type: text/plain; charset=UTF-8\r\n";
  content += "Content-Transfer-Encoding: base64\r\n";
  content += "\r\n";
  content += body.toUtf8().toBase64() + "\r\n";
  content += "\r\n";
  content += "--" + boundary + "\r\n";
  content += "Content-Type: text/html; charset=UTF-8\r\n";
  content += "Content-Transfer-Encoding: base64\r\n";
  content += "\r\n";

  // 构建 HTML 格式邮件
  QString htmlBody = body;
  htmlBody.replace("\n", "<br>");
  QString html = "<html><body style='font-family: Microsoft YaHei, sans-serif; "
                 "padding: 20px;'>";
  html += "<h2 style='color: #007AFF;'>TaskDispatcher</h2>";
  html += "<hr style='border: 1px solid #E5E5E5;'>";
  html += "<div style='margin-top: 20px;'>" + htmlBody + "</div>";
  html += "<hr style='border: 1px solid #E5E5E5; margin-top: 30px;'>";
  html += "<p style='color: #999; font-size: 12px;'>此邮件由 TaskDispatcher "
          "自动发送，请勿回复。</p>";
  html += "</body></html>";
  content += html.toUtf8().toBase64() + "\r\n";
  content += "\r\n";
  content += "--" + boundary + "--\r\n";

  return content;
}
