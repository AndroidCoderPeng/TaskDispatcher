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

enum class TaskStatus {
  Pending,   // 未执行
  Running,   // 执行中
  Completed, // 已执行
  Failed     // 执行失败
};

struct Task {
  qint32 id;
  QDateTime scheduledTime;
  TaskStatus status = TaskStatus::Pending;
};

#endif // GLOBALDEFINITION_HPP