#-------------------------------------------------
#
# Project created by QtCreator 2010-07-06T12:35:08
#
#-------------------------------------------------

TARGET = PluginTest
TEMPLATE = app
CONFIG   += qt
QT       += core gui opengl

include ( ../Common.pri )

LIBS += -lCommon -lNotedPlugin

SOURCES += main.cpp
