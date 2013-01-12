TEMPLATE = lib
include ( ../Common.pri )

CONFIG += qt uic
QT       += core gui opengl

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

