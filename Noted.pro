TEMPLATE = subdirs
system(echo $$CONFIG)
!android {
SUBDIRS = \
    EventCompiler \
    ExampleEventCompiler \
    Audio
!cross: SUBDIRS += \
    contrib \
	NotedPlugin \
	Grapher \
    ExamplePlugin \
    EventsEditor \
    Noted \
    TestPlugin \
    PluginTest \
    Test
}
