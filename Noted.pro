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
	VisualizerPlugin \
	EventsEditor \
    Noted \
    TestPlugin \
    PluginTest \
	NotedTest
}

tidy()
