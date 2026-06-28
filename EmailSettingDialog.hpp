#ifndef EMAILSETTINGDIALOG_HPP
#define EMAILSETTINGDIALOG_HPP

#include "GlobalDefinition.hpp"

#include <QDialog>
#include <QPair>

namespace Ui {
class EmailSettingDialog;
}

class EmailSettingDialog : public QDialog {
  Q_OBJECT

public:
  explicit EmailSettingDialog(QWidget *parent = nullptr);
  ~EmailSettingDialog();

  void onSaveButtonClicked();

  void onCancelButtonClicked();

  QPair<bool, EmailConfig> getInputValue() const;

private:
  Ui::EmailSettingDialog *ui;
  EmailConfig cfg;
  bool accepted = false;
};

#endif // EMAILSETTINGDIALOG_HPP
