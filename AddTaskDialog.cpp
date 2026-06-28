#include "AddTaskDialog.hpp"
#include "ui_AddTaskDialog.h"

AddTaskDialog::AddTaskDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddTaskDialog) {
  ui->setupUi(this);

  connect(ui->saveButton, &QPushButton::clicked, this,
          &AddTaskDialog::onSaveButtonClicked);
  connect(ui->cancelButton, &QPushButton::clicked, this,
          &AddTaskDialog::onCancelButtonClicked);
}

void AddTaskDialog::onSaveButtonClicked() {
  // QString messageTitle = ui->messageTitleView->text();
  // if (messageTitle.isEmpty()) {
  //     messageTitle = "指令执行结果通知";
  // }
  // cfg.messageTitle = messageTitle;

  // const QString wxKey = ui->wxKeyView->toPlainText().trimmed();
  // if (wxKey.isEmpty()) {
  //     QMessageBox::warning(this, "警告", "企业微信webhook Key不能为空");
  //     return;
  // }
  // cfg.wxKey = wxKey;

  accepted = true;
  accept();
}

void AddTaskDialog::onCancelButtonClicked() { close(); }

QPair<bool, Task> AddTaskDialog::getInputValue() const {
  return {accepted, task};
}

AddTaskDialog::~AddTaskDialog() { delete ui; }
