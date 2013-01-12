TEMPLATE = app
include ( ../Common.pri )

CONFIG   += qt
QT       += core gui opengl

linux: LIBS += -lboost_system

SOURCES += main.cpp
