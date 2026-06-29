#ifndef TASKITEMWIDGET_HPP
#define TASKITEMWIDGET_HPP

#include <QLabel>
#include <QTime>
#include <QWidget>

#include "GlobalDefinition.hpp"

class TaskItemWidget : public QWidget {
  Q_OBJECT

public:
  explicit TaskItemWidget(const Task &task, QWidget *parent = nullptr);

  QSize sizeHint() const override;

  /// 设置实际执行时间（计划时间 + 随机偏移）
  void setActualTime(const QTime &time);

  qint32 taskId() const;

private:
  void setupUi();

  Task mTask;
  QLabel *mTimeLabelPtr;
  QLabel *mActualTimeLabelPtr;
};

#endif // TASKITEMWIDGET_HPP
