#ifndef TOASTWIDGET_HPP
#define TOASTWIDGET_HPP

#include <QFrame>
#include <QLabel>
#include <QTimer>

enum class ToastLevel { Info, Warning, Error };

/**
 * 非阻塞 Toast 消息提示组件
 * 在父窗口顶部居中浮层显示，自动消失，不阻塞任务逻辑
 */
class ToastWidget : public QFrame {
  Q_OBJECT

public:
  explicit ToastWidget(QWidget *parent = nullptr);
  ~ToastWidget() override = default;

  /// 在 parent 上显示一条 Toast 消息，durationMs 毫秒后自动消失
  static void showInfo(QWidget *parent, const QString &message,
                       int durationMs = 3000);
  static void showWarning(QWidget *parent, const QString &message,
                          int durationMs = 4000);
  static void showError(QWidget *parent, const QString &message,
                        int durationMs = 5000);

private:
  static void showToast(QWidget *parent, const QString &message, ToastLevel level,
                        int durationMs);

  void setupUi(ToastLevel level, const QString &message, int durationMs);
  void reposition();

  QLabel *iconLabelPtr = nullptr;
  QLabel *messageLabelPtr = nullptr;
  QTimer *dismissTimerPtr = nullptr;
  ToastLevel level;

  static constexpr int TOAST_MIN_WIDTH = 200;
  static constexpr int TOAST_MAX_WIDTH = 600;
  static constexpr int TOAST_MARGIN = 12;
  static constexpr int TOAST_SPACING = 8;
};

#endif // TOASTWIDGET_HPP
