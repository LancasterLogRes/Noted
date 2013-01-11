TEMPLATE = lib
include ( ../Common.pri )

LIBS += -lportaudio
SOURCES += \
        Playback.cpp \
        Capture.cpp \
    Common.cpp

HEADERS += \
        Capture.h \
        Playback.h \
    Common.h
