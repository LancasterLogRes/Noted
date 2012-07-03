# You need to sort out these paths.

DESTDIR = $$OUT_PWD/../built

CONFIG += no_include_pwd
CONFIG -= uic

!force_shared:!force_static {
    embedded: CONFIG += force_static
    !embedded: CONFIG += force_shared
}

CONFIG(release, debug|release) {
    CONFIG += release ndebug
    CONFIG -= debug
    profile: QMAKE_CXXFLAGS += -g3
    !profile: QMAKE_CXXFLAGS += -g0 -fomit-frame-pointer -Wl,-strip-all
    QMAKE_CXXFLAGS += -DRELEASE -O2
    system (echo "Release build")
}

CONFIG(debug, debug|release) {
    CONFIG -= release ndebug
    QMAKE_CXXFLAGS += -DDEBUG -g3 -fno-inline -O0 -Wall
    !win32: QMAKE_CXXFLAGS += -fPIC
    system (echo "Debug build")
}

force_shared {
    CONFIG -= create_prl link_prl static
    CONFIG += shared dll dylib
    QMAKE_CXXFLAGS += -DLIGHTBOX_SHARED_LIBRARY=1
    system (echo "Shared build")
}

force_static {
    CONFIG += create_prl link_prl static
    CONFIG -= shared dll dylib
    QMAKE_CXXFLAGS += -DLIGHTBOX_STATIC_LIBRARY=1
    LIBS += -static
    system (echo "Static build")
}

QMAKE_CXXFLAGS += -ffast-math -pipe -fexceptions
!mac: QMAKE_CXXFLAGS += -std=c++0x
#crosscompilation: QMAKE_CXXFLAGS += -march=btver1 -mtune=btver1
#!crosscompilation: QMAKE_CXXFLAGS += -march=native
QMAKE_CXXFLAGS += -march=native
QMAKE_CXXFLAGS_WARN_ON += -Wno-parentheses
mac: QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$IN_PWD
# For macports
mac: LIBS += -L/opt/local/lib

win32 {
	BOOST = C:/boost_1_50_0
    FFTW = C:/Lightbox/fftw
    PORTAUDIO = C:/Lightbox/portaudio
	LIBS += -L$$BOOST/stage/lib -L$$FFTW -L$$PORTAUDIO/lib/.libs
    INCLUDEPATH += $$BOOST $$FFTW $$PORTAUDIO/include
}

LIBS += -L$$DESTDIR -Wl,-rpath,$$DESTDIR
DEPENDPATH = $INCLUDEPATH

