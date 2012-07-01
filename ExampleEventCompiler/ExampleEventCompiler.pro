#-------------------------------------------------
#
# Project created by QtCreator 2011-12-19T12:08:43
#
#-------------------------------------------------

CONFIG -= qt
TARGET = ExampleEventCompiler
TEMPLATE = lib

CONFIG += force_shared
LIBS += -lCommon -lEventCompiler
include ( ../Common.pri )

SOURCES += ExampleEventCompiler.cpp
HEADERS += ExampleEventCompiler.h
