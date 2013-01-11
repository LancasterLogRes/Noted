TEMPLATE = lib
include ( ../Common.pri )

QT       += core gui opengl
CONFIG += qt uic

LIBS += -lCommon -lNotedPlugin
SOURCES += Grapher.cpp
HEADERS  += Grapher.h
