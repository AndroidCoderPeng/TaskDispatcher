#include "TaskItemWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>

TaskItemWidget::TaskItemWidget(const Task &task, QWidget *parent)
    : QWidget(parent), mTask(task) {
  setupUi();
}

void TaskItemWidget::setupUi() {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(12);

  // 计划时间标签
  mTimeLabelPtr = new QLabel(mTask.scheduledTime.toString("HH:mm:ss"), this);
  QFont timeFont = mTimeLabelPtr->font();
  timeFont.setPointSize(12);
  timeFont.setBold(true);
  mTimeLabelPtr->setFont(timeFont);

  // 实际执行时间标签（初始为空）
  mActualTimeLabelPtr = new QLabel(QStringLiteral("—"), this);
  mActualTimeLabelPtr->setAlignment(Qt::AlignCenter);
  mActualTimeLabelPtr->setMinimumWidth(72);
  QFont actualFont = mActualTimeLabelPtr->font();
  actualFont.setPointSize(9);
  mActualTimeLabelPtr->setFont(actualFont);

  layout->addWidget(mTimeLabelPtr);
  layout->addStretch();
  layout->addWidget(mActualTimeLabelPtr);
}

QSize TaskItemWidget::sizeHint() const {
  const int h = mTimeLabelPtr->sizeHint().height() +
                layout()->contentsMargins().top() +
                layout()->contentsMargins().bottom();
  return QSize(QWidget::sizeHint().width(), qMax(h, 56));
}

void TaskItemWidget::setActualTime(const QTime &time) {
  if (!time.isValid() || time == QTime(0, 0)) {
    mActualTimeLabelPtr->setText(QStringLiteral("—"));
    mActualTimeLabelPtr->setStyleSheet("");
  } else {
    mActualTimeLabelPtr->setText(time.toString("HH:mm"));
    mActualTimeLabelPtr->setStyleSheet(
        "QLabel { color: #007AFF; font-weight: bold; }");
  }
}

qint32 TaskItemWidget::taskId() const { return mTask.id; }
