QT += core testlib
QT -= gui
TEMPLATE = app
TARGET = test_checksumdat
DEPENDPATH += .
INCLUDEPATH += ../../src
CONFIG += debug console
QMAKE_CXXFLAGS += -std=c++17

HEADERS += ../../src/checksumdat.h

SOURCES += test_checksumdat.cpp \
            ../../src/checksumdat.cpp
