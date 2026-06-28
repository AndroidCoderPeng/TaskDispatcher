#ifndef ADDTASKDIALOG_HPP
#define ADDTASKDIALOG_HPP

#include <QDialog>

#include "GlobalDefinition.hpp"

namespace Ui {
class AddTaskDialog;
}

class AddTaskDialog : public QDialog {
  Q_OBJECT

public:
  explicit AddTaskDialog(QWidget *parent = nullptr);
  ~AddTaskDialog();

  void onSaveButtonClicked();

  void onCancelButtonClicked();

  QPair<bool, Task> getInputValue() const;

private:
  Ui::AddTaskDialog *ui;
  Task task;
  bool accepted = false;
};

#endif // ADDTASKDIALOG_HPP
