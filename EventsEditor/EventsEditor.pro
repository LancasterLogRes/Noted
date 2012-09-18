#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = EventsEditor
TEMPLATE = lib
QT       += core gui opengl
CONFIG += qt

include ( ../Common.pri )

LIBS += -lCommon -lNotedPlugin

SOURCES += EventsEditScene.cpp \
    EventsEditor.cpp \
    PeriodItem.cpp \
    StreamEventItem.cpp \
    SustainItem.cpp \
    SyncPointItem.cpp \
    AttackItem.cpp

HEADERS  += EventsEditScene.h \
    EventsEditor.h \
    PeriodItem.h \
    StreamEventItem.h \
    SustainItem.h \
    DoEventTypes.h \
    SyncPointItem.h \
    AttackItem.h

