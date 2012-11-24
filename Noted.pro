TEMPLATE = subdirs
system(echo $$CONFIG)
!android {
SUBDIRS = \
    Common \
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
