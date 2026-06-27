#ifndef WXSETTINGDIALOG_HPP
#define WXSETTINGDIALOG_HPP

#include "GlobalDefinition.hpp"

#include <QDialog>

namespace Ui {
class WxSettingDialog;
}

class WxSettingDialog : public QDialog {
  Q_OBJECT

public:
  explicit WxSettingDialog(QWidget *parent = nullptr);
  ~WxSettingDialog();

  void onSaveCommandButtonClicked();

  void onCancelButtonClicked();

  QPair<bool, WxConfig> getInputValue() const;

private:
  Ui::WxSettingDialog *ui;
  WxConfig cfg;
  bool accepted = false;
};

#endif // WXSETTINGDIALOG_HPP
