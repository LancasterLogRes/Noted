TEMPLATE = subdirs
include (Common.pri)
!android {
SUBDIRS = \
    Audio
native: SUBDIRS += \
	Viz \
	Grapher \
	NotedPlugin \
	ExamplePlugin \
	ExampleVizPlugin \
	EventsEditor \
    Noted \
    TestPlugin \
    PluginTest \
	NotedTest
}

tidy()
