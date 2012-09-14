DESTDIR = $$OUT_PWD/../built

CONFIG += no_include_pwd
CONFIG -= uic
DEFINES += "LIGHTBOX_TARGET_NAME=$$TARGET"

crosscompilation: CONFIG += force_static
!crosscompilation: CONFIG += force_shared

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
	DEFINES += LIGHTBOX_SHARED_LIBRARY
	system (echo "Shared build")
}

force_static {
	CONFIG += create_prl link_prl static
	CONFIG -= shared dll dylib
	DEFINES += LIGHTBOX_STATIC_LIBRARY
        LIBS += -static
	system (echo "Static build")
}

profile {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS += -pg
}

QMAKE_CXXFLAGS += -ffast-math -pipe -fexceptions
mac: QMAKE_CXXFLAGS +=  -std=c++11
!mac: QMAKE_CXXFLAGS +=  -std=c++0x
crosscompilation {
    QMAKE_CXXFLAGS += -march=amdfam10 -O2 -pipe -mno-3dnow -mcx16 -mpopcnt -msse3 -msse4a -mmmx
    DEFINES += LIGHTBOX_CROSSCOMPILATION
}
!crosscompilation {
    QMAKE_CXXFLAGS += -march=native
    DEFINES += LIGHTBOX_NATIVE
}

QMAKE_CXXFLAGS_WARN_ON += -Wno-parentheses

INCLUDEPATH += $$IN_PWD
# For macports
mac: LIBS += -L/opt/local/lib

win32 {
	# Dependent on your configuration.
	BOOST = C:/boost_1_50_0
	FFTW = C:/Lightbox/fftw
	PORTAUDIO = C:/Lightbox/portaudio
	GLEW = C:/Lightbox/glew-1.8.0
	RESAMPLE = C:/Lightbox/libresample-0.1.3
	SNDFILE = C:/Lightbox/libsndfile
	LIBS += -L$$BOOST/stage/lib -L$$FFTW -L$$PORTAUDIO/lib/.libs -L$$GLEW/lib -L$$RESAMPLE -L$$SNDFILE/lib
	INCLUDEPATH += $$BOOST $$FFTW $$PORTAUDIO/include $$GLEW/include $$RESAMPLE/include $$SNDFILE/include
        FFTW3_LIBS = -lfftw3f-3
        SNDFILE_LIBS = $$SNDFILE/lib/libsndfile-1.lib
        GL_LIBS += -lOpenGL32 -lGLU32 -lGLEW32
}
!win32 {
        FFTW3_LIBS = -lfftw3f
        SNDFILE_LIBS = -lsndfile
        GL_LIBS += -lGL -lGLU -lGLEW
}

LIBS += -L$$DESTDIR -Wl,-rpath,$$DESTDIR
DEPENDPATH = $INCLUDEPATH
