QT += core gui serialport charts widgets network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
win32: { CONFIG += c++20 }
unix: {QMAKE_CXXFLAGS +=           \
  -std=c++2b                       \
  -Wno-deprecated-copy             \
  -DD_develop                      \
  -Wno-deprecated-enum-enum-conversion 
}



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
FORMS += pwb.ui
win32: {LIBS += -lws2_32 -lsetupapi}
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib/ -lQt6Mqtt
