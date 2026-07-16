#include "MailSender.hpp"

#include "ConfigStore.hpp"
#include "Logger.hpp"

#include <QCoreApplication>

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
          QOverload<QAbstractSocket::SocketError>::of(
              &QAbstractSocket::errorOccurred),
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
    return;
  }

  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    Logger::Tag("MailSender").w("Socket is busy, aborting previous connection");
    socketPtr->abort();
  }

  pendingSubject = subject == nullptr ? config.emailTitle : subject;
  pendingBody = body;
  smtpStep = 0;
  multiLineBuffer.clear();

  Logger::Tag("MailSender").dFmt("Connecting to %s:%d", SMTP_HOST, SMTP_PORT);
  socketPtr->connectToHostEncrypted(SMTP_HOST, SMTP_PORT);
  timeoutTimerPtr->start(SMTP_TIMEOUT_MS);
}

void MailSender::sendAttachmentEmail(const QString &subject,
                                     const QString &body,
                                     const QByteArray bytes) {
  if (!loadEmailConfig()) {
    Logger::Tag("MailSender").e("Email config not found or incomplete");
    return;
  }

  if (bytes.isEmpty()) {
    Logger::Tag("MailSender").e("Attachment data is empty");
    return;
  }

  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    Logger::Tag("MailSender").w("Socket is busy, aborting previous connection");
    socketPtr->abort();
  }

  pendingSubject = subject == nullptr ? config.emailTitle : subject;
  pendingBody = body;
  pendingAttachmentData = bytes;
  pendingAttachmentFileName = "image.png";
  smtpStep = 0;
  multiLineBuffer.clear();

  Logger::Tag("MailSender")
      .dFmt("Connecting to %s:%d (with attachment, %d bytes)", SMTP_HOST,
            SMTP_PORT, bytes.size());
  socketPtr->connectToHostEncrypted(SMTP_HOST, SMTP_PORT);
  timeoutTimerPtr->start(SMTP_TIMEOUT_MS);
}

void MailSender::onConnected() {
  Logger::Tag("MailSender").i("TCP connected, waiting for SSL handshake...");
}

void MailSender::onEncrypted() {
  Logger::Tag("MailSender").i("SSL handshake completed");
}

void MailSender::onReadyRead() {
  timeoutTimerPtr->start(SMTP_TIMEOUT_MS);

  while (socketPtr->canReadLine()) {
    QString line = QString::fromUtf8(socketPtr->readLine()).trimmed();

    int code = line.leftRef(3).toInt();
    if (code == 0) {
      // 无法解析响应码的行，跳过
      continue;
    }

    // SMTP 多行响应：4xx/5xx 开头表示错误，直接失败
    if (code >= 400) {
      fail("服务器返回错误: " + line);
      return;
    }

    // 检查是否为多行响应的最终行（code + 空格开头）
    if (!isFinalLine(line, expectedCode)) {
      // 中间行，跳过
      continue;
    }

    // 最终行到达，处理响应
    handleSmtpResponse(code);
  }
}

bool MailSender::isFinalLine(const QString &line, int expectedCode) {
  Q_UNUSED(expectedCode);

  if (line.length() < 4)
    return true;
  // SMTP 多行响应：中间行是 "250-xxx"，最终行是 "250 xxx"
  QChar sep = line.at(3);
  return sep == ' ';
}

void MailSender::handleSmtpResponse(int code) {
  Q_UNUSED(code);

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
    QString mailContent;
    if (!pendingAttachmentData.isEmpty()) {
      mailContent = buildAttachmentMailContent(pendingSubject, pendingBody,
                                               pendingAttachmentData,
                                               pendingAttachmentFileName);
    } else {
      mailContent = buildMailContent(pendingSubject, pendingBody);
    }
    Logger::Tag("MailSender").i("Sending mail content...");
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
    // 带附件时数据量大，重置超时给服务端处理时间
    timeoutTimerPtr->start(SMTP_TIMEOUT_MS);
  } break;

  case 8: // 邮件内容已接收 (250)
    smtpStep = 9;
    expectedCode = 221;
    sendCommand("QUIT");
    break;

  case 9: // QUIT 响应 (221)
    timeoutTimerPtr->stop();
    socketPtr->disconnectFromHost();
    pendingAttachmentData.clear();
    pendingAttachmentFileName.clear();
    Logger::Tag("MailSender").i("Email sent successfully!");
    break;

  default:
    break;
  }
}

void MailSender::sendCommand(const QString &cmd) {
  socketPtr->write(cmd.toUtf8() + "\r\n");
  socketPtr->flush();
}

void MailSender::fail(const QString &reason) {
  timeoutTimerPtr->stop();
  if (socketPtr->state() != QAbstractSocket::UnconnectedState) {
    socketPtr->disconnectFromHost();
  }
  pendingAttachmentData.clear();
  pendingAttachmentFileName.clear();
  Logger::Tag("MailSender")
      .eFmt("Send failed: %s", reason.toStdString().c_str());
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

  QString content;

  // 构建邮件头部
  content += "From: " + from + "\r\n";
  content += "To: " + to + "\r\n";
  content += "Subject: =?UTF-8?B?" + subject.toUtf8().toBase64() + "?=\r\n";
  content += "MIME-Version: 1.0\r\n";
  content += "Content-Type: text/plain; charset=UTF-8\r\n";
  content += "Content-Transfer-Encoding: base64\r\n";
  content += "\r\n";

  // 邮件正文内容使用 Base64 编码
  content += body.toUtf8().toBase64() + "\r\n";

  return content;
}

QString MailSender::buildAttachmentMailContent(const QString &subject,
                                               const QString &body,
                                               const QByteArray &attachmentData,
                                               const QString &fileName) const {
  QString from = config.senderEmail.trimmed();
  QString to = config.receiverEmail.trimmed();

  const QString boundary = "----=_TaskDispatcher_Mixed_0001";

  QString content;

  // 邮件头部
  content += "From: " + from + "\r\n";
  content += "To: " + to + "\r\n";
  content += "Subject: =?UTF-8?B?" + subject.toUtf8().toBase64() + "?=\r\n";
  content += "MIME-Version: 1.0\r\n";
  content += "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n";
  content += "\r\n";

  // ===== 纯文本正文部分 =====
  content += "--" + boundary + "\r\n";
  content += "Content-Type: text/plain; charset=UTF-8\r\n";
  content += "Content-Transfer-Encoding: base64\r\n";
  content += "\r\n";
  content += body.toUtf8().toBase64() + "\r\n";

  // ===== 附件部分 =====
  content += "--" + boundary + "\r\n";
  content += "Content-Type: image/png; name=\"" + fileName + "\"\r\n";
  content +=
      "Content-Disposition: attachment; filename=\"" + fileName + "\"\r\n";
  content += "Content-Transfer-Encoding: base64\r\n";
  content += "\r\n";

  // 附件 base64 编码，每 76 字符换行（MIME 规范）
  const QByteArray base64Data = attachmentData.toBase64();
  for (int i = 0; i < base64Data.size(); i += 76) {
    content += QString::fromUtf8(base64Data.mid(i, 76)) + "\r\n";
  }

  // 结束边界
  content += "--" + boundary + "--\r\n";

  return content;
}
