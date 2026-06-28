#include "TaskItemWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>

TaskItemWidget::TaskItemWidget(const Task &task, QWidget *parent)
    : QWidget(parent), mTask(task) {
  setupUi();
  applyStatusStyle(task.status);
}

void TaskItemWidget::setupUi() {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);

  // 时间标签
  mTimeLabelPtr = new QLabel(mTask.scheduledTime.toString("HH:mm:ss"), this);
  QFont timeFont = mTimeLabelPtr->font();
  timeFont.setPointSize(12);
  timeFont.setBold(true);
  mTimeLabelPtr->setFont(timeFont);

  // 状态标签
  mStatusLabelPtr = new QLabel(this);
  mStatusLabelPtr->setAlignment(Qt::AlignCenter);
  mStatusLabelPtr->setMinimumWidth(56);
  QFont statusFont = mStatusLabelPtr->font();
  statusFont.setPointSize(9);
  mStatusLabelPtr->setFont(statusFont);
  // 根据最长的状态文本预留宽度："执行失败"四个字
  mStatusLabelPtr->setText("执行失败");
  mStatusLabelPtr->adjustSize();
  mStatusLabelPtr->setMinimumWidth(mStatusLabelPtr->width() + 4);

  layout->addWidget(mTimeLabelPtr);
  layout->addStretch();
  layout->addWidget(mStatusLabelPtr);
}

QSize TaskItemWidget::sizeHint() const {
  // 让 item 高度根据内部控件自适应
  const int h = mTimeLabelPtr->sizeHint().height() +
                layout()->contentsMargins().top() +
                layout()->contentsMargins().bottom();
  return QSize(QWidget::sizeHint().width(), qMax(h, 56));
}

void TaskItemWidget::updateStatus(TaskStatus status) {
  mTask.status = status;
  applyStatusStyle(status);
}

qint32 TaskItemWidget::taskId() const { return mTask.id; }

void TaskItemWidget::applyStatusStyle(TaskStatus status) {
  switch (status) {
  case TaskStatus::Pending:
    mStatusLabelPtr->setText("未执行");
    mStatusLabelPtr->setStyleSheet(
        "QLabel { background: #E0E0E0; color: #616161; border-radius: 4px; "
        "padding: 2px 6px; }");
    break;
  case TaskStatus::Running:
    mStatusLabelPtr->setText("执行中");
    mStatusLabelPtr->setStyleSheet(
        "QLabel { background: #BBDEFB; color: #1565C0; border-radius: 4px; "
        "padding: 2px 6px; }");
    break;
  case TaskStatus::Completed:
    mStatusLabelPtr->setText("已执行");
    mStatusLabelPtr->setStyleSheet(
        "QLabel { background: #C8E6C9; color: #2E7D32; border-radius: 4px; "
        "padding: 2px 6px; }");
    break;
  case TaskStatus::Failed:
    mStatusLabelPtr->setText("执行失败");
    mStatusLabelPtr->setStyleSheet(
        "QLabel { background: #FFCDD2; color: #C62828; border-radius: 4px; "
        "padding: 2px 6px; }");
    break;
  }
}
