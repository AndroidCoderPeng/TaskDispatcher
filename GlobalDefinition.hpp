#ifndef GLOBALDEFINITION_HPP
#define GLOBALDEFINITION_HPP

#include <QDateTime>
#include <QString>

enum class WebSocketState { RUNNING, SHUTDOWN };

struct EmailConfig {
  QString emailTitle;
  QString senderEmail;
  QString authCode;
  QString receiverEmail;
};

struct WxConfig {
  QString messageTitle;
  QString wxKey;
};

struct ResetTaskConfig {
  QString time;
};

struct Task {
  qint32 id;
  QDateTime scheduledTime;
};

#endif // GLOBALDEFINITION_HPP