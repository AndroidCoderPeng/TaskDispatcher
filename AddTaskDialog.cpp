#include "AddTaskDialog.hpp"
#include "ui_AddTaskDialog.h"

#include <QDate>
#include <QDateTime>

AddTaskDialog::AddTaskDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AddTaskDialog) {
  ui->setupUi(this);

  connect(ui->saveButton, &QPushButton::clicked, this,
          &AddTaskDialog::onSaveButtonClicked);
  connect(ui->cancelButton, &QPushButton::clicked, this,
          &AddTaskDialog::onCancelButtonClicked);
}

void AddTaskDialog::onSaveButtonClicked() {
  const QTime time = ui->timeEdit->time();
  task.scheduledTime = QDateTime(QDate::currentDate(), time);
  accepted = true;
  accept();
}

void AddTaskDialog::onCancelButtonClicked() { close(); }

QPair<bool, Task> AddTaskDialog::getInputValue() const {
  return {accepted, task};
}

AddTaskDialog::~AddTaskDialog() { delete ui; }
