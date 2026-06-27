#include "EmailSettingDialog.hpp"
#include "ui_EmailSettingDialog.h"

#include <QMessageBox>

EmailSettingDialog::EmailSettingDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::EmailSettingDialog) {
  ui->setupUi(this);

  connect(ui->saveButton, &QPushButton::clicked, this,
          &EmailSettingDialog::onSaveCommandButtonClicked);
  connect(ui->cancelButton, &QPushButton::clicked, this,
          &EmailSettingDialog::onCancelButtonClicked);
}

void EmailSettingDialog::onSaveCommandButtonClicked() {
  QString emailTitle = ui->emailTitleView->text();
  if (emailTitle.isEmpty()) {
    emailTitle = "指令执行结果通知";
  }
  cfg.emailTitle = emailTitle;

  const QString senderEmail = ui->senderEmailView->text();
  if (senderEmail.isEmpty()) {
    QMessageBox::warning(this, "警告", "发件人邮箱不能为空");
    return;
  }

  if (!senderEmail.endsWith("@qq.com")) {
    QMessageBox::warning(this, "警告", "发件人邮箱仅支持QQ邮箱");
    return;
  }
  cfg.senderEmail = senderEmail;

  const QString authCode = ui->authCodeView->text();
  if (authCode.isEmpty()) {
    QMessageBox::warning(this, "警告", "授权码不能为空");
    return;
  }
  cfg.authCode = authCode;

  const QString receiverEmail = ui->receiverEmailView->text();
  if (receiverEmail.isEmpty()) {
    QMessageBox::warning(this, "警告", "收件人邮箱不能为空");
    return;
  }
  cfg.receiverEmail = receiverEmail;

  accepted = true;
  accept();
}

void EmailSettingDialog::onCancelButtonClicked() { close(); }

QPair<bool, EmailConfig> EmailSettingDialog::getInputValue() const {
  return {accepted, cfg};
}

EmailSettingDialog::~EmailSettingDialog() { delete ui; }
