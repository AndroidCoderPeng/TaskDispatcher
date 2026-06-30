TARGET = TaskDispatcher
TEMPLATE = app
win32 {
    RC_ICONS = application.ico
}

QT     += core gui widgets network sql concurrent

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AddTaskDialog.cpp \
    ChinaHolidayManager.cpp \
    ConfigStore.cpp \
    EmailSettingDialog.cpp \
    ImageProcessor.cpp \
    Logger.cpp \
    DispatcherApplication.cpp \
    MailSender.cpp \
    ProcessExecutor.cpp \
    ResetTaskSettingDialog.cpp \
    TaskExecutor.cpp \
    TaskItemWidget.cpp \
    TaskStore.cpp \
    ToastWidget.cpp \
    WxMessageSender.cpp \
    WxSettingDialog.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    AddTaskDialog.hpp \
    ChinaHolidayManager.hpp \
    ConfigStore.hpp \
    EmailSettingDialog.hpp \
    GlobalDefinition.hpp \
    ImageProcessor.hpp \
    Logger.hpp \
    DispatcherApplication.hpp \
    MailSender.hpp \
    MainWindow.hpp \
    ProcessExecutor.hpp \
    ResetTaskSettingDialog.hpp \
    TaskExecutor.hpp \
    TaskItemWidget.hpp \
    TaskStore.hpp \
    ToastWidget.hpp \
    WxMessageSender.hpp \
    WxSettingDialog.hpp

FORMS += \
    AddTaskDialog.ui \
    EmailSettingDialog.ui \
    MainWindow.ui \
    ResetTaskSettingDialog.ui \
    WxSettingDialog.ui

RESOURCES += \
    font.qrc \
    image.qrc \
    style.qrc

DISTFILES += \
    script/amd_x86_64/build_release.sh \
    script/windows_x86_64/TaskDispatcher.iss \
    script/windows_x86_64/build_release.bat \
    script/windows_x86_64/README.txt \
    README.md

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
