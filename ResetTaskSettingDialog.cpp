#include "ResetTaskSettingDialog.hpp"
#include "ui_ResetTaskSettingDialog.h"

#include "ConfigStore.hpp"

#include <QMessageBox>
#include <QTime>

ResetTaskSettingDialog::ResetTaskSettingDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ResetTaskSettingDialog) {
  ui->setupUi(this);
  const auto obj = ConfigStore::get().load("resetTaskConfig");
  if (!obj.isEmpty()) {
    cfg.time = obj.value("time").toString();

    ui->timeEdit->setTime(QTime::fromString(cfg.time, "HH:mm:ss"));
  }

  connect(ui->saveButton, &QPushButton::clicked, this,
          &ResetTaskSettingDialog::onSaveButtonClicked);
  connect(ui->cancelButton, &QPushButton::clicked, this,
          &ResetTaskSettingDialog::onCancelButtonClicked);
}

void ResetTaskSettingDialog::onSaveButtonClicked() {
  cfg.time = ui->timeEdit->time().toString("HH:mm:ss");
  accepted = true;
  accept();
}

void ResetTaskSettingDialog::onCancelButtonClicked() { close(); }

QPair<bool, ResetTaskConfig> ResetTaskSettingDialog::getInputValue() const {
  return {accepted, cfg};
}

ResetTaskSettingDialog::~ResetTaskSettingDialog() { delete ui; }
