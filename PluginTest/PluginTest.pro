TEMPLATE = app
include ( ../Common.pri )

CONFIG   += qt
QT       += core gui opengl

LIBS += -lCommon -lNotedPlugin
linux: LIBS += -lboost_system

SOURCES += main.cpp
