TARGET = Noted
TEMPLATE = app
QT       += core gui xml opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += qt

include ( ../Common.pri )

win32: RC_FILE = Noted.rc
CONFIG += uic
LIBS += -lAudio -lCommon -lEventsEditor -lNotedPlugin $$FFTW3_LIBS $$SNDFILE_LIBS -lresample -lcontrib $$GL_LIBS

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
	NotedGLWidget.cpp \
    Cache.cpp

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
	PropertiesEditor.h \
    Cache.h

FORMS    += Noted.ui

OTHER_FILES += \
	TODO.txt \
	Noted.rc \
	SpectraView.frag \
	SpectraView.vert

RESOURCES += \
	Noted.qrc
