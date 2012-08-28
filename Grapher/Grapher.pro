#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = Grapher
TEMPLATE = lib
QT       += core gui opengl
CONFIG += qt

include ( ../Common.pri )

LIBS += -lCommon -lNotedPlugin

SOURCES += Grapher.cpp

HEADERS  += Grapher.h
