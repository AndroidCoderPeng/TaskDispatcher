TARGET = TaskDispatcher
TEMPLATE = app
win32 {
    RC_ICONS = application.ico
}

QT     += core gui widgets network websockets sql concurrent

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AddTaskDialog.cpp \
    ChinaHolidayManager.cpp \
    ConfigStore.cpp \
    EmailSettingDialog.cpp \
    Logger.cpp \
    DispatcherApplication.cpp \
    MailSender.cpp \
    ProcessExecutor.cpp \
    ResetTaskSettingDialog.cpp \
    TaskExecutor.cpp \
    TaskItemWidget.cpp \
    TaskStore.cpp \
    ToastWidget.cpp \
    WebSocketObserver.cpp \
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
    WebSocketObserver.hpp \
    WsProtocol.hpp \
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

# 拷贝 OpenSSL DLL（Windows 下 Qt SSL 需要）
win32 {
    # 常见 OpenSSL 安装路径
    OPENSSL_PATHS = \
        "C:/Program Files/OpenSSL-Win64/bin" \
        "C:/Program Files/OpenSSL-Win32/bin" \
        "C:/OpenSSL-Win64/bin" \
        "C:/vcpkg/installed/x64-windows/bin"

    for(OPENSSL_PATH, OPENSSL_PATHS) {
        exists($$OPENSSL_PATH/libcrypto-1_1-x64.dll) {
            message("Found OpenSSL at $$OPENSSL_PATH")
            # 拷贝到构建输出目录
            QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$replace(OPENSSL_PATH, /, \\)\\libcrypto-1_1-x64.dll\" \"$$replace(OUT_PWD, /, \\)\\release\\\" > nul 2>&1)
            QMAKE_POST_LINK += && $$quote(cmd /c copy /y \"$$replace(OPENSSL_PATH, /, \\)\\libssl-1_1-x64.dll\" \"$$replace(OUT_PWD, /, \\)\\release\\\" > nul 2>&1)
            break()
        }
        exists($$OPENSSL_PATH/libcrypto-1_1.dll) {
            message("Found OpenSSL (32-bit) at $$OPENSSL_PATH")
            QMAKE_POST_LINK += $$quote(cmd /c copy /y \"$$replace(OPENSSL_PATH, /, \\)\\libcrypto-1_1.dll\" \"$$replace(OUT_PWD, /, \\)\\release\\\" > nul 2>&1)
            QMAKE_POST_LINK += && $$quote(cmd /c copy /y \"$$replace(OPENSSL_PATH, /, \\)\\libssl-1_1.dll\" \"$$replace(OUT_PWD, /, \\)\\release\\\" > nul 2>&1)
            break()
        }
    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
