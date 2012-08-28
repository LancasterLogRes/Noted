TEMPLATE = subdirs
system(echo $$CONFIG)
SUBDIRS = \
    Common \
    EventCompiler \
    ExampleEventCompiler \
    Audio
!crosscompilation: SUBDIRS += \
    contrib \
    Grapher \
    NotedPlugin \
    ExamplePlugin \
    EventsEditor \
    Noted \
    TestPlugin \
    PluginTest \
    Test
