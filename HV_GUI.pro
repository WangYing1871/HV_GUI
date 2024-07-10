QT       += core gui serialport charts widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32: {
  CONFIG += c++20
}
unix: {QMAKE_CXXFLAGS +=           \
  -std=c++2b                       \
  -Wno-deprecated-copy             \
  -DD_develop                      \
  -Wno-deprecated-enum-enum-conversion 
}


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    modbus-data.c \
    modbus-rtu.c \
    modbus-tcp.c \
    modbus.c \
    pwb.cpp \
    switch.cpp \
    util.cpp

HEADERS += \
    modbus-private.h \
    modbus-rtu-private.h \
    modbus-rtu.h \
    modbus-tcp-private.h \
    modbus-tcp.h \
    modbus-version.h \
    modbus.h \
    modbus_pwb_ifc.h \
    pwb.h \
    switch.h \
    util.h

FORMS += \
    pwb.ui
win32: {LIBS += -lws2_32 -lsetupapi}
