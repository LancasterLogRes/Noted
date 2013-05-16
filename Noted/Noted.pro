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
	SpectrumView.cpp \
	DeltaSpectrumView.cpp \
	MetaSpectrumView.cpp \
	EventsView.cpp \
	WorkerThread.cpp \
	WaveView.cpp \
	WaveWindowView.cpp \
	SpectraView.cpp \
	CurrentView.cpp \
	Grapher.cpp \
	WaveOverview.cpp \
	NotedBase.cpp \
	CompileEventsView.cpp \
		PropertiesEditor.cpp \
	NotedGLWidget.cpp \
	GraphView.cpp \
    TimelinesItem.cpp \
    ComputeMan.cpp \
    LibraryMan.cpp

HEADERS  += Noted.h \
	WaveView.h \
	SpectraView.h \
	EventsView.h \
	WaveWindowView.h \
	SpectrumView.h \
	DeltaSpectrumView.h \
	ProcessEventCompiler.h \
	MetaSpectrumView.h \
	WorkerThread.h \
	CurrentView.h \
	Grapher.h \
	WaveOverview.h \
	NotedGLWidget.h \
	NotedBase.h \
	CompileEvents.h \
	CollateEvents.h \
	CompileEventsView.h \
	PropertiesEditor.h \
    GraphView.h \
	Global.h \
    TimelinesItem.h \
    ComputeMan.h \
    LibraryMan.h

FORMS    += Noted.ui

OTHER_FILES += \
	TODO.txt \
	Noted.rc \
	SpectraView.frag \
	SpectraView.vert \
    Noted.qml

RESOURCES += \
	Noted.qrc
