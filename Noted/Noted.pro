TEMPLATE = app
include ( ../Common.pri )

win32: RC_FILE = Noted.rc
CONFIG += qt uic
QT       += core gui xml opengl widgets quick
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
LIBS += $$FFTW3_LIBS $$SNDFILE_LIBS -lresample -lcontrib $$GL_LIBS
linux: LIBS += -lX11 -lboost_system

SOURCES += main.cpp\
	Noted.cpp \
	EventsView.cpp \
	WorkerThread.cpp \
	CurrentView.cpp \
	Grapher.cpp \
	CompileEventsView.cpp \
	PropertiesEditor.cpp \
	NotedGLWidget.cpp \
	GraphView.cpp \
    TimelinesItem.cpp \
    ComputeMan.cpp \
    LibraryMan.cpp \
    AudioMan.cpp \
    ViewMan.cpp \
    EventsMan.cpp \
	GraphMan.cpp \
	FileAudioStream.cpp \
    GraphItem.cpp \
    TimelineItem.cpp

HEADERS  += Noted.h \
	EventsView.h \
	ProcessEventCompiler.h \
	WorkerThread.h \
	CurrentView.h \
	Grapher.h \
	NotedGLWidget.h \
	CompileEvents.h \
	CollateEvents.h \
	CompileEventsView.h \
	PropertiesEditor.h \
    GraphView.h \
	Global.h \
    TimelinesItem.h \
    ComputeMan.h \
    LibraryMan.h \
    AudioMan.h \
    ViewMan.h \
    EventsMan.h \
	GraphMan.h \
	FileAudioStream.h \
    GraphItem.h \
    TimelineItem.h

FORMS    += Noted.ui

OTHER_FILES += \
	TODO.txt \
	Noted.rc \
    Noted.qml \
    GraphTimeline.qml \
    GraphSpec.qml \
    ControlPanel.qml \
    Button.qml

RESOURCES += \
	Noted.qrc
