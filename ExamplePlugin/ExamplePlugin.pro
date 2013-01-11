TEMPLATE = lib
CONFIG += force_shared
include ( ../Common.pri )

CONFIG += qt uic
QT += core gui

LIBS += -lCommon -lNotedPlugin
!win32: LIBS += -lGL -lGLU
win32: LIBS += -lOpenGL32 -lGLU32

SOURCES += ExamplePlugin.cpp
HEADERS += ExamplePlugin.h

