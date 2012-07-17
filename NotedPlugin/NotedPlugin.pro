#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = NotedPlugin
TEMPLATE = lib
QT       += core gui opengl
CONFIG += qt

include ( ../Common.pri )

LIBS += -lCommon -lEventCompiler

SOURCES += Timeline.cpp \
	PrerenderedTimeline.cpp \
	NotedFace.cpp \
	NotedPlugin.cpp \
	Prerendered.cpp \
	AcausalAnalysis.cpp \
	CausalAnalysis.cpp \
    QGLWidgetProxy.cpp

HEADERS  += Timeline.h \
	PrerenderedTimeline.h \
	NotedFace.h \
	NotedPlugin.h \
	Prerendered.h \
	CausalAnalysis.h \
	AcausalAnalysis.h \
	AuxLibraryFace.h \
    QGLWidgetProxy.h
