#-------------------------------------------------
#
# Project created by QtCreator 2011-12-08T10:09:13
#
#-------------------------------------------------

CONFIG -= qt
TARGET = Common
TEMPLATE = lib
include ( ../Common.pri )

LIBS += -lfftw3f

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
