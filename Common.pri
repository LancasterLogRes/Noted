# Noted/Common.pri
# Noted! common project include file.
# This top bit houses the configuration section which you do need to know about.

# Beginning of configuration:

# If you're on Windows, you'll need to set up the Portaudio, libresample &
# libsndfile project paths:
win32 {
	PORTAUDIO = C:/Lightbox/portaudio
	RESAMPLE = C:/Lightbox/libresample-0.1.3
	SNDFILE = C:/Lightbox/libsndfile
}

# End of configurable part
##############################################################################

LIGHTBOX_ROOT_PROJECT = Noted
LIGHTBOX_USES_PROJECTS = Lightbox
include ( Noted.depends )
include ( ../Lightbox/Common.pri )

win32 {
	INCLUDEPATH += $$PORTAUDIO/include $$RESAMPLE/include $$SNDFILE/include
	LIBS += -L$$PORTAUDIO/lib/.libs -L$$RESAMPLE -L$$SNDFILE/lib
	SNDFILE_LIBS = $$SNDFILE/lib/libsndfile-1.lib
}

!win32 {
	FFTW3_LIBS = -lfftw3f
	SNDFILE_LIBS = -lsndfile
}

#system (echo $$DESTDIR)
