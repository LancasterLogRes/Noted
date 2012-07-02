TARGET = Audio
CONFIG	-= qt
TEMPLATE = lib

LIBS += -lportaudio

include ( ../Common.pri )

SOURCES += \
        Playback.cpp \
        Capture.cpp \
    Common.cpp

HEADERS += \
        Capture.h \
        Playback.h \
    Common.h
