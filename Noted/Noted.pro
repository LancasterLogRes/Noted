#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = Noted
TEMPLATE = app
QT       += core gui xml
CONFIG += qt

include ( ../Common.pri )

CONFIG += uic
LIBS += -lAudio -lCommon -lEventsEditor -lNotedPlugin
linux: LIBS += -lfftw3f
win32: LIBS += -lfftw3f-3

SOURCES += main.cpp\
    Noted.cpp \
    SpectrumView.cpp \
    DeltaSpectrumView.cpp \
    MetaSpectrumView.cpp \
    EventsView.cpp \
    DataView.cpp \
    Cursor.cpp \
    WorkerThread.cpp \
    WaveView.cpp \
    WaveWindowView.cpp \
    SpectraView.cpp \
    CurrentView.cpp \
    Grapher.cpp \
    WaveOverview.cpp \
    NotedBase.cpp \
    Page.cpp \
    Pager.cpp \
    CompileEventsView.cpp

HEADERS  += Noted.h \
    WaveView.h \
    SpectraView.h \
    EventsView.h \
    WaveWindowView.h \
    SpectrumView.h \
    DeltaSpectrumView.h \
    ProcessEventCompiler.h \
    MetaSpectrumView.h \
    DataView.h \
    Cursor.h \
    WorkerThread.h \
    CurrentView.h \
    Grapher.h \
    WaveOverview.h \
    NotedGLWidget.h \
    NotedBase.h \
    Page.h \
    Pager.h \
    CompileEvents.h \
    CollateEvents.h \
    CompileEventsView.h

FORMS    += Noted.ui

OTHER_FILES += \
    DemoRig.fixture \
    TODO.txt
