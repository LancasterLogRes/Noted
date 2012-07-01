# You need to sort out these paths.

DESTDIR = $$OUT_PWD/../built

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
    QMAKE_CXXFLAGS += -DDEBUG -g3 -fno-inline -O0 -Wall -fPIC
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

QMAKE_CXXFLAGS += -std=c++0x -ffast-math -pipe -fexceptions
embedded: QMAKE_CXXFLAGS += -march=btver1 -mtune=btver1
!embedded: QMAKE_CXXFLAGS += -march=native
QMAKE_CXXFLAGS_WARN_ON += -Wno-parentheses

INCLUDEPATH += $$IN_PWD
DEPENDPATH = $INCLUDEPATH
LIBS += -L$$DESTDIR -Wl,-rpath,$$DESTDIR
