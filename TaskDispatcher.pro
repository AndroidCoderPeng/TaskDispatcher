TARGET = TaskDispatcher
TEMPLATE = app
win32 {
    RC_ICONS = application.ico
}

QT     += core gui widgets network websockets sql

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ConfigStore.cpp \
    EmailSettingDialog.cpp \
    Logger.cpp \
    DispatcherApplication.cpp \
    WebSocketObserver.cpp \
    WxSettingDialog.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    ConfigStore.hpp \
    EmailSettingDialog.hpp \
    GlobalDefinition.hpp \
    Logger.hpp \
    DispatcherApplication.hpp \
    MainWindow.hpp \
    WebSocketObserver.hpp \
    WxSettingDialog.hpp

FORMS += \
    EmailSettingDialog.ui \
    MainWindow.ui \
    WxSettingDialog.ui

RESOURCES += \
    font.qrc \
    image.qrc \
    style.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
