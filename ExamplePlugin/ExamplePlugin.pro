#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = ExamplePlugin
TEMPLATE = lib
CONFIG += qt
QT += core gui

CONFIG += force_shared
include ( ../Common.pri )
LIBS += -lCommon -lNotedPlugin
!win32: LIBS += -lGL -lGLU
win32: LIBS += -lOpenGL32 -lGLU32

SOURCES += ExamplePlugin.cpp
HEADERS += ExamplePlugin.h

