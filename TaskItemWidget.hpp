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

  /// 刷新主题样式（不重建 UI，保留实际时间等状态）
  void refreshThemeStyle();

  qint32 taskId() const;

  static bool isDarkTheme();
  static void setDarkTheme(bool dark);

private:
  void setupUi();

  Task mTask;
  QLabel *mScheduledPrefixPtr;
  QLabel *mTimeLabelPtr;
  QLabel *mActualPrefixPtr;
  QLabel *mActualTimeLabelPtr;

  static bool sDarkTheme;
};

#endif // TASKITEMWIDGET_HPP
