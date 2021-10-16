TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp
LIBS += -lX11
LIBS += -lGL
LIBS += -lGLU
LIBS += -lpthread

HEADERS += \
    basic.h
