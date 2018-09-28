#-------------------------------------------------
#
# Project created by QtCreator 2018-09-21T20:23:59
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NV14-updater
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    firmwarerequest.cpp \
    remotefileinfo.cpp \
    dfu/dfu.c \
    dfu/stm32mem.c \
    dfu_manager.cpp \
    dfu/intel_hex.c \
    aboutdialog.cpp

HEADERS += \
        mainwindow.h \
    firmwarerequest.h \
    remotefileinfo.h \
    dfu/dfu.h \
    dfu/stm32mem.h \
    dfu_manager.h \
    dfu/intel_hex.h \
    dfu/dfu-bool.h \
    aboutdialog.h

FORMS += \
        mainwindow.ui \
    aboutdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

win32:RC_ICONS += resources/icon.ico
win32: LIBS += -L$$PWD/../libusb-1.0.22/MinGW32/static -llibusb-1.0
unix: LIBS += -L/usr/local/lib -lusb-1.0

win32: INCLUDEPATH += $$PWD/../libusb-1.0.22/include

win32:!win32-g++: DEPENDPATH += $$PWD/../libusb-1.0.22/MS32/static
else:win32-g++: DEPENDPATH += $$PWD/../llibusb-1.0.22/MinGW32/static

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../libusb-1.0.22/MS32/static/libusb-1.0.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../libusb-1.0.22/MinGW32/static/libusb-1.0.a
