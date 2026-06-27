#include "WxSettingDialog.hpp"
#include "ui_WxSettingDialog.h"

#include "ConfigStore.hpp"

#include <QMessageBox>

WxSettingDialog::WxSettingDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::WxSettingDialog) {
  ui->setupUi(this);

  const auto obj = ConfigStore::get().load("wxConfig");
  if (!obj.isEmpty()) {
    cfg.messageTitle = obj.value("messageTitle").toString();
    cfg.wxKey = obj.value("wxKey").toString();

    ui->messageTitleView->setText(cfg.messageTitle);
    ui->wxKeyView->setText(cfg.wxKey);
  }

  connect(ui->saveButton, &QPushButton::clicked, this,
          &WxSettingDialog::onSaveCommandButtonClicked);
  connect(ui->cancelButton, &QPushButton::clicked, this,
          &WxSettingDialog::onCancelButtonClicked);
}

void WxSettingDialog::onSaveCommandButtonClicked() {
  QString messageTitle = ui->messageTitleView->text();
  if (messageTitle.isEmpty()) {
    messageTitle = "指令执行结果通知";
  }
  cfg.messageTitle = messageTitle;

  const QString wxKey = ui->wxKeyView->text();
  if (wxKey.isEmpty()) {
    QMessageBox::warning(this, "警告", "企业微信webhook Key不能为空");
    return;
  }
  cfg.wxKey = wxKey;

  accepted = true;
  accept();
}

void WxSettingDialog::onCancelButtonClicked() { close(); }

QPair<bool, WxConfig> WxSettingDialog::getInputValue() const {
  return {accepted, cfg};
}

WxSettingDialog::~WxSettingDialog() { delete ui; }
