#ifndef RESETTASKSETTINGDIALOG_HPP
#define RESETTASKSETTINGDIALOG_HPP

#include "GlobalDefinition.hpp"

#include <QDialog>

namespace Ui {
class ResetTaskSettingDialog;
}

class ResetTaskSettingDialog : public QDialog {
  Q_OBJECT

public:
  explicit ResetTaskSettingDialog(QWidget *parent = nullptr);
  ~ResetTaskSettingDialog();

  void onSaveButtonClicked();

  void onCancelButtonClicked();

  QPair<bool, ResetTaskConfig> getInputValue() const;

private:
  Ui::ResetTaskSettingDialog *ui;
  ResetTaskConfig cfg;
  bool accepted = false;
};

#endif // RESETTASKSETTINGDIALOG_HPP
