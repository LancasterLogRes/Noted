#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = TestPlugin
TEMPLATE = lib
CONFIG += qt
QT += core gui opengl xml

CONFIG += force_shared
include ( ../Common.pri )
LIBS += -lCommon -lNotedPlugin
linux: LIBS += -lGLU

SOURCES += TestPlugin.cpp
HEADERS += TestPlugin.h

