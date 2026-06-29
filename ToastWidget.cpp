#include "ToastWidget.hpp"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QPixmap>
#include <QTimer>

ToastWidget::ToastWidget(QWidget *parent)
    : QFrame(parent), level(ToastLevel::Info) {
  setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
  setAttribute(Qt::WA_TransparentForMouseEvents, false);
  setAttribute(Qt::WA_ShowWithoutActivating);
  setAttribute(Qt::WA_DeleteOnClose);
}

void ToastWidget::showToast(QWidget *parent, const QString &message,
                            ToastLevel level, int durationMs) {
  if (!parent)
    return;

  // 关闭父窗口中所有已有的 toast，避免堆积
  const auto children = parent->findChildren<ToastWidget *>();
  for (auto *child : children) {
    child->close();
  }

  auto *toast = new ToastWidget(parent);
  toast->setupUi(level, message, durationMs);
  toast->QWidget::show();
}

void ToastWidget::showInfo(QWidget *parent, const QString &message,
                           int durationMs) {
  showToast(parent, message, ToastLevel::Info, durationMs);
}

void ToastWidget::showWarning(QWidget *parent, const QString &message,
                              int durationMs) {
  showToast(parent, message, ToastLevel::Warning, durationMs);
}

void ToastWidget::showError(QWidget *parent, const QString &message,
                            int durationMs) {
  showToast(parent, message, ToastLevel::Error, durationMs);
}

void ToastWidget::setupUi(ToastLevel level, const QString &message,
                          int durationMs) {
  this->level = level;

  QColor color;
  QString iconPath;
  switch (level) {
  case ToastLevel::Info:
    color = QColor(0x07, 0x7A, 0xFF); // 蓝色
    iconPath = QStringLiteral(":/toast_info.png");
    break;
  case ToastLevel::Warning:
    color = QColor(0xFF, 0x98, 0x00); // 橙色
    iconPath = QStringLiteral(":/toast_warning.png");
    break;
  case ToastLevel::Error:
    color = QColor(0xE5, 0x39, 0x35); // 红色
    iconPath = QStringLiteral(":/toast_error.png");
    break;
  }

  setStyleSheet(QString("QFrame { border: 2px solid %1; border-radius: 6px; "
                        "background: white; }")
                    .arg(color.name()));

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 10, 12, 10);
  layout->setSpacing(10);

  // 图标
  iconLabelPtr = new QLabel(this);
  iconLabelPtr->setFixedSize(24, 24);
  iconLabelPtr->setScaledContents(true);
  iconLabelPtr->setPixmap(QPixmap(iconPath).scaled(24, 24, Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
  iconLabelPtr->setStyleSheet(
      QString("QLabel { background: transparent; border: none; }"));
  layout->addWidget(iconLabelPtr);

  // 消息文本
  messageLabelPtr = new QLabel(message, this);
  messageLabelPtr->setWordWrap(true);
  messageLabelPtr->setStyleSheet(
      QString("QLabel { color: %1; font-size: 13px; background: transparent; "
              "border: none; }")
          .arg(color.name()));
  messageLabelPtr->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
  layout->addWidget(messageLabelPtr);

  // 自适应宽度：边距(12+12) + 图标(24) + 间距(10) = 58px
  {
    constexpr int kFixedOverhead = 58;
    QFont font;
    font.setPixelSize(13);
    const QFontMetrics fm(font);
    const int textWidth = fm.horizontalAdvance(message);
    const int idealWidth = textWidth + kFixedOverhead;
    const int clampedWidth =
        qBound(TOAST_MIN_WIDTH, idealWidth, TOAST_MAX_WIDTH);
    setFixedWidth(clampedWidth);
  }

  adjustSize();
  reposition();

  // 自动消失定时器
  dismissTimerPtr = new QTimer(this);
  dismissTimerPtr->setSingleShot(true);
  connect(dismissTimerPtr, &QTimer::timeout, this, &QWidget::close);
  dismissTimerPtr->start(durationMs);
}

void ToastWidget::reposition() {
  if (!parentWidget())
    return;

  const int parentW = parentWidget()->width();
  const int parentH = parentWidget()->height();

  const int x = (parentW - width()) / 2;
  const int y = (parentH - height()) / 2;
  move(x, y);
}
