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
	Prerendered.cpp \
	Cache.cpp \
	DataMan.cpp \
	DataSet.cpp \
    ViewManFace.cpp \
    EventsManFace.cpp \
    EventsStore.cpp \
    GraphManFace.cpp \
    AudioManFace.cpp \
    ComputeAnalysis.cpp

HEADERS  += Timeline.h \
	PrerenderedTimeline.h \
	NotedFace.h \
	NotedPlugin.h \
	CausalAnalysis.h \
	AcausalAnalysis.h \
	AuxLibraryFace.h \
    QGLWidgetProxy.h \
    Prerendered.h \
	Library.h \
	Cache.h \
	DataMan.h \
	DataSet.h \
    ComputeManFace.h \
    AudioManFace.h \
    GraphManFace.h \
    LibraryManFace.h \
    Common.h \
    ViewManFace.h \
    EventsManFace.h \
    EventsStore.h \
	JobSource.h \
	NotedComputeRegistrar.h \
    NotedFeeder.h \
    ComputeAnalysis.h 
