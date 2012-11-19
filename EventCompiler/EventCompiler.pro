#-------------------------------------------------
#
# Project created by QtCreator 2011-12-08T10:09:13
#
#-------------------------------------------------

CONFIG -= qt
TARGET = EventCompiler
TEMPLATE = lib
include ( ../Common.pri )

LIBS += -lCommon

HEADERS += StreamEvent.h \
    EventCompilerLibrary.h \
    EventCompilerImpl.h \
    EventCompiler.h \
    Preprocessors.h \
    Track.h \
    EventType.h \
    Character.h

SOURCES += \
    Preprocessors.cpp \
    Track.cpp \
    StreamEvent.cpp
