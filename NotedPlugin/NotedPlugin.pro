TEMPLATE = lib
include ( ../Common.pri )

CONFIG += qt
QT       += core gui opengl widgets

SOURCES += Timeline.cpp \
	PrerenderedTimeline.cpp \
	NotedFace.cpp \
	NotedPlugin.cpp \
	AcausalAnalysis.cpp \
	CausalAnalysis.cpp \
    QGLWidgetProxy.cpp \
    Prerendered.cpp

HEADERS  += Timeline.h \
	PrerenderedTimeline.h \
	NotedFace.h \
	NotedPlugin.h \
	CausalAnalysis.h \
	AcausalAnalysis.h \
	AuxLibraryFace.h \
    QGLWidgetProxy.h \
    Prerendered.h \
    Library.h
