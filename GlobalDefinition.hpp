#ifndef GLOBALDEFINITION_HPP
#define GLOBALDEFINITION_HPP

#include <QDateTime>
#include <QString>

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