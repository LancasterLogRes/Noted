TEMPLATE = subdirs
system(echo $$CONFIG)
!android {
SUBDIRS = \
    EventCompiler \
    ExampleEventCompiler \
    Audio
!crosscompilation: SUBDIRS += \
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
