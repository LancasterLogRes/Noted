LIGHTBOX_ROOT_PROJECT = Noted
LIGHTBOX_USES_PROJECTS = Lightbox
include ( Noted.depends )
include ( ../Lightbox/Common.pri )

FTDI_LIBS = -lftdi -L$$TP/libftdi/src -Wl,-rpath,$$TP/libftdi/src -lusb-1.0
FTDI_INCLUDEPATH = $$TP/libftdi/src

libusb {
    pi {
        LIBS += -lusb-1.0
        INCLUDEPATH += /usr/include/libusb-1.0
    }
    unix: !mac: x86 {
        CONFIG		+= link_pkgconfig
        PKGCONFIG	+= libusb-1.0
    }
    win32 {
        LIBS += -L$$TP/libusb-1.0.9/libusb/.libs -Wl,-rpath,$$TP/libusb-1.0.9/libusb/.libs -lusb-1.0
        INCLUDEPATH += $$TP/libusb-1.0.9/libusb
    }
}

system (echo $$DESTDIR)
