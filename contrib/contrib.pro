CONFIG -= qt
TARGET = contrib
TEMPLATE = lib

include ( ../Common.pri )
LIBS += $$GL_LIBS -lGLEW
SOURCES += glsl.cpp
HEADERS += glsl.h
