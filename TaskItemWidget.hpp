#ifndef TASKITEMWIDGET_HPP
#define TASKITEMWIDGET_HPP

#include <QLabel>
#include <QWidget>

#include "GlobalDefinition.hpp"

class TaskItemWidget : public QWidget {
  Q_OBJECT

public:
  explicit TaskItemWidget(const Task &task, QWidget *parent = nullptr);

  QSize sizeHint() const override;

  void updateStatus(TaskStatus status);

  qint32 taskId() const;

private:
  void setupUi();
  void applyStatusStyle(TaskStatus status);

  Task mTask;
  QLabel *mTimeLabelPtr;
  QLabel *mStatusLabelPtr;
};

#endif // TASKITEMWIDGET_HPP
