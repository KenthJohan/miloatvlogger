TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS += -Wno-unused-function

SOURCES += atvlogger.c


LIBS += -lmosquitto
LIBS += -lm
LIBS += -lpthread


