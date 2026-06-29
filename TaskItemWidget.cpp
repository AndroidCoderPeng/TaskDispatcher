#include "TaskItemWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>

bool TaskItemWidget::sDarkTheme = false;

// ====== Badge 样式定义 ======

namespace {

QString scheduledBadgeStyle() {
  return TaskItemWidget::isDarkTheme()
             ? QStringLiteral("QLabel {"
                              "  background: #3A3A50;"
                              "  color: #A0A0A0;"
                              "  border: 1px solid #505068;"
                              "  border-radius: 4px;"
                              "  padding: 1px 4px;"
                              "}")
             : QStringLiteral("QLabel {"
                              "  background: #E8E8E8;"
                              "  color: #666666;"
                              "  border: 1px solid #D0D0D0;"
                              "  border-radius: 4px;"
                              "  padding: 1px 4px;"
                              "}");
}

QString actualBadgeStyle() {
  return TaskItemWidget::isDarkTheme()
             ? QStringLiteral("QLabel {"
                              "  background: #1B3A2A;"
                              "  color: #4CAF50;"
                              "  border: 1px solid #2E5A3A;"
                              "  border-radius: 4px;"
                              "  padding: 1px 4px;"
                              "}")
             : QStringLiteral("QLabel {"
                              "  background: #E8F5E9;"
                              "  color: #2E7D32;"
                              "  border: 1px solid #A5D6A7;"
                              "  border-radius: 4px;"
                              "  padding: 1px 4px;"
                              "}");
}

} // namespace

// ====== TaskItemWidget ======

TaskItemWidget::TaskItemWidget(const Task &task, QWidget *parent)
    : QWidget(parent), mTask(task) {
  setupUi();
}

void TaskItemWidget::setupUi() {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(4);

  QFont prefixFont = font();
  prefixFont.setPointSize(10);

  // 计划时间前缀 badge
  mScheduledPrefixPtr = new QLabel(QStringLiteral("计划时间"), this);
  mScheduledPrefixPtr->setFont(prefixFont);
  mScheduledPrefixPtr->setStyleSheet(scheduledBadgeStyle());

  // 计划时间值
  mTimeLabelPtr = new QLabel(mTask.scheduledTime.toString("HH:mm:ss"), this);
  QFont timeFont = mTimeLabelPtr->font();
  timeFont.setPointSize(12);
  timeFont.setBold(true);
  mTimeLabelPtr->setFont(timeFont);

  // 实际时间前缀 badge
  mActualPrefixPtr = new QLabel(QStringLiteral("实际时间"), this);
  mActualPrefixPtr->setFont(prefixFont);
  mActualPrefixPtr->setStyleSheet(actualBadgeStyle());

  // 实际执行时间值（初始为空）
  mActualTimeLabelPtr = new QLabel(QStringLiteral("--:--:--"), this);
  mActualTimeLabelPtr->setAlignment(Qt::AlignCenter);
  mActualTimeLabelPtr->setMinimumWidth(42);
  QFont actualFont = mActualTimeLabelPtr->font();
  actualFont.setPointSize(12);
  mActualTimeLabelPtr->setFont(actualFont);

  layout->addWidget(mScheduledPrefixPtr);
  layout->addWidget(mTimeLabelPtr);
  layout->addStretch();
  layout->addWidget(mActualPrefixPtr);
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
    mActualTimeLabelPtr->setText(QStringLiteral("--:--:--"));
    mActualTimeLabelPtr->setStyleSheet("");
  } else {
    mActualTimeLabelPtr->setText(time.toString("HH:mm:ss"));
    mActualTimeLabelPtr->setStyleSheet(
        "QLabel { color: #007AFF; font-weight: bold; }");
  }
}

void TaskItemWidget::refreshThemeStyle() {
  mScheduledPrefixPtr->setStyleSheet(scheduledBadgeStyle());
  mActualPrefixPtr->setStyleSheet(actualBadgeStyle());
}

qint32 TaskItemWidget::taskId() const { return mTask.id; }

bool TaskItemWidget::isDarkTheme() { return sDarkTheme; }

void TaskItemWidget::setDarkTheme(bool dark) { sDarkTheme = dark; }
