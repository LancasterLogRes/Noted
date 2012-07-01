#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = NotedPlugin
TEMPLATE = lib
QT       += core gui
CONFIG += qt

include ( ../Common.pri )

LIBS += -lCommon -lEventCompiler

SOURCES += Timeline.cpp \
    PrerenderedTimeline.cpp \
    NotedFace.cpp \
    NotedPlugin.cpp \
    Prerendered.cpp \
    GLView.cpp \
    AcausalAnalysis.cpp \
    CausalAnalysis.cpp

HEADERS  += Timeline.h \
    PrerenderedTimeline.h \
    NotedFace.h \
    NotedPlugin.h \
    Prerendered.h \
    GLView.h \
    CausalAnalysis.h \
    AcausalAnalysis.h \
    AuxLibraryFace.h
