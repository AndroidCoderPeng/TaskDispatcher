#ifndef GLOBALDEFINITION_HPP
#define GLOBALDEFINITION_HPP

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

#endif // GLOBALDEFINITION_HPP