#-------------------------------------------------
#
# Project created by QtCreator 2010-07-07T12:57:53
#
#-------------------------------------------------

TARGET = Noted
TEMPLATE = app
QT       += core gui xml opengl
CONFIG += qt

include ( ../Common.pri )

win32: RC_FILE = Noted.rc
CONFIG += uic
LIBS += -lAudio -lCommon -lEventsEditor -lNotedPlugin -l$$FFTW3_LIB -lsndfile -lresample

SOURCES += main.cpp\
	Noted.cpp \
	SpectrumView.cpp \
	DeltaSpectrumView.cpp \
	MetaSpectrumView.cpp \
	EventsView.cpp \
	DataView.cpp \
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
	CompileEventsView.cpp \
        PropertiesEditor.cpp \
    NotedGLWidget.cpp

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
	CompileEventsView.h \
    PropertiesEditor.h

FORMS    += Noted.ui

OTHER_FILES += \
	TODO.txt \
	Noted.rc

RESOURCES += \
	Noted.qrc
