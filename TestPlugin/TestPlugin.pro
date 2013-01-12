TEMPLATE = lib
CONFIG += force_shared
include ( ../Common.pri )

CONFIG += qt uic
QT += core gui opengl xml

linux: LIBS += -lGLU

SOURCES += TestPlugin.cpp
HEADERS += TestPlugin.h

