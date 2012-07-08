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

LIBS += -l$$FFTW3F_LIB

SOURCES += Common.cpp \
    FFTW.cpp \
    Maths.cpp \
    StreamIO.cpp \
    Global.cpp \
    Color.cpp \
    Peaks.cpp \
    Time.cpp
HEADERS += Common.h Global.h \
    Time.h \
    Properties.h \
    GraphParameters.h \
    WavHeader.h \
    Algorithms.h \
    Color.h \
    Trivial.h \
    BoolArray.h \
    FFTW.h \
    UnitTesting.h \
    Maths.h \
    StreamIO.h \
    Statistics.h \
    Peaks.h \
    Flags.h
