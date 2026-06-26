#ifndef DISPATCHERAPPLICATION_H
#define DISPATCHERAPPLICATION_H

#pragma once
#include <QApplication>

#include "MainWindow.hpp"

class DispatcherApplication : public QApplication {
  Q_OBJECT

public:
  DispatcherApplication(int &argc, char **argv);
  ~DispatcherApplication();

private:
  MainWindow *mainWindowPtr;

  void initMainWindow();
};

#endif // DISPATCHERAPPLICATION_H
