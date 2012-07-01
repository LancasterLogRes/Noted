# You need to sort out these paths.

DESTDIR = $$OUT_PWD/../built

debug {
    !force_shared:!force_static: CONFIG += force_shared
    CONFIG -= release ndebug
    QMAKE_CXXFLAGS += -DDEBUG -g3 -fno-inline -O0 -Wall -fPIC
    system (echo "Debug build")
}
!debug|ndebug {
    !force_shared:!force_static: CONFIG += force_static
    CONFIG += release ndebug
    CONFIG -= debug
    profile: QMAKE_CXXFLAGS += -g3
    !profile: QMAKE_CXXFLAGS += -g0 -fomit-frame-pointer -Wl,-strip-all
    QMAKE_CXXFLAGS += -DRELEASE -O2
    system (echo "Release build")
}

force_shared {
    CONFIG -= create_prl link_prl static
    CONFIG += shared dll dylib
    system (echo "Shared build")
}
force_static {
    CONFIG += create_prl link_prl static
    CONFIG -= shared dll dylib
    LIBS += -static
    system (echo "Static build")
}

# -march=btver1 -mtune=btver1
QMAKE_CXXFLAGS += -std=c++0x -ffast-math -pipe -fexceptions -march=native
QMAKE_CXXFLAGS_WARN_ON += -Wno-parentheses
#-msse -msse2 -msse3
#-fvisibility-inlines-hidden \
#-fvisibility=hidden \

INCLUDEPATH += $$IN_PWD
DEPENDPATH = $INCLUDEPATH
LIBS += -L$$DESTDIR -Wl,-rpath,$$DESTDIR
