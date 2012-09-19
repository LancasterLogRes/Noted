#-------------------------------------------------
#
# Project created by QtCreator 2011-12-08T10:09:13
#
#-------------------------------------------------

CONFIG -= qt
TARGET = Common
TEMPLATE = lib
include ( ../Common.pri )

# required for non-windows platforms, it seems...
!win32: LIBS += -lboost_system

# for windows, it doesn't seem to find libboost_system, so we do it manually.
win32 {
	debug: LIBS += -lboost_system-mgw46-mt-d-1_50
	release: LIBS += -lboost_system-mgw46-mt-1_50
}

LIBS += $$FFTW3_LIBS

SOURCES += Common.cpp \
    FFTW.cpp \
    Maths.cpp \
    StreamIO.cpp \
    Global.cpp \
    RGBA.cpp \
    Color.cpp \
    Peaks.cpp \
    Time.cpp
HEADERS += Common.h Global.h \
    Time.h \
    GraphParameters.h \
    WavHeader.h \
    Algorithms.h \
    RGBA.h \
    Color.h \
    Trivial.h \
    BoolArray.h \
    FFTW.h \
    UnitTesting.h \
    Maths.h \
    StreamIO.h \
    Statistics.h \
    Peaks.h \
    Flags.h \
    DoTypes.h \
    MemberCollection.h \
    MemberMap.h \
    Members.h \
    GL.h \
    RGBA.h
