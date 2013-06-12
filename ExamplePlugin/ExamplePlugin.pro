TEMPLATE = lib
CONFIG += force_shared
include ( ../Common.pri )

CONFIG += qt uic
QT += core gui widgets

SOURCES += ExamplePlugin.cpp
HEADERS += ExamplePlugin.h

