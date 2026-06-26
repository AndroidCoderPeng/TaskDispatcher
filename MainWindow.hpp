#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

#include "GlobalDefinition.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

  void bindIpAddresses(const QList<QString> &ips);

public slots:
  void slotServerStateChanged(const WebSocketState &state);

  void slotDataReceived(const QString &message);

private slots:
  void onOpenSocketButton();

private:
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_HPP
