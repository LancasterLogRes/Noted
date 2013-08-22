TEMPLATE = lib
CONFIG += force_shared
include ( ../Common.pri )

CONFIG += qt uic
QT += core gui widgets

!win32: LIBS += -lGL -lGLU
win32: LIBS += -lOpenGL32 -lGLU32

SOURCES += \
    VisualizerPlugin.cpp
HEADERS += \
    VisualizerPlugin.h

