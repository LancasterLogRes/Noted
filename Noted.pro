TEMPLATE = subdirs
include (Common.pri)
!android {
SUBDIRS = \
    Audio
native: SUBDIRS += \
    contrib \
	Grapher \
	NotedPlugin \
    ExamplePlugin \
    EventsEditor \
    Noted \
    TestPlugin \
    PluginTest \
	NotedTest
}

tidy()
