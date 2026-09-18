QT       += core gui widgets

CONFIG   += c++17
TARGET    = FlappyBird
TEMPLATE  = app

SOURCES += \
    main.cpp \
    gamewidget.cpp \
    sfx.cpp

HEADERS += \
    gamewidget.h \
    sfx.h

RESOURCES += \
    resources.qrc

# WebAssembly synthesises sound through the browser's Web Audio API and needs no
# Qt module. Desktop uses QAudioSink where Qt Multimedia is available; without it
# the build still succeeds and simply stays silent.
!wasm:qtHaveModule(multimedia) {
    QT += multimedia
    DEFINES += FB_DESKTOP_AUDIO
}
