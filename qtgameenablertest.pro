# Copyright (c) 2011 Nokia Corporation.

QT += core gui

TARGET = qtgameenablertest
TEMPLATE = app
VERSION = 1.1.1

# Comment the following line out for better performance. Using the definition
# enables debug logging which is convenient for locating problems in the code
# but is also very costly in terms of performance.
DEFINES += GE_DEBUG

INCLUDEPATH += ./src ./test_src

SOURCES += \
    src/GameWindow.cpp \
    src/GEAudioBuffer.cpp \
    src/GEAudioBufferPlayInstance.cpp \
    src/GEAudioMixer.cpp \
    src/GEAudioOut.cpp \
    src/GEInterfaces.cpp \
    test_src/main.cpp \
    test_src/mygamewindow.cpp \
    test_src/mygamewindoweventfilter.cpp

HEADERS  += \
    src/GameWindow.h \
    src/GEAudioBuffer.h \
    src/GEAudioBufferPlayInstance.h \
    src/GEAudioMixer.h \
    src/GEAudioOut.h \
    src/GEInterfaces.h \
    src/trace.h \
    test_src/mygamewindow.h \
    test_src/mygamewindoweventfilter.h

OTHER_FILES +=
    
CONFIG += mobility
MOBILITY =


symbian {
    message(Symbian build)

    CONFIG += mobility
    MOBILITY += multimedia

    TARGET = QtGameEnablerTest
    TARGET.UID3 = 0xee69dad8
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000

    ICON = icons/qtgameenabler.svg

    # For checking the current profile.
    LIBS += -lcentralrepository

    LIBS += -llibEGL -llibGLESv2 -lcone -leikcore -lavkon

    # Uncomment the following define to enable a very ugly hack to set the
    # volume level on Symbian devices higher. By default, on Symbian, the volume
    # level is very low when audio is played using QAudioOutput class. For now,
    # this ugly hack is the only way to set the volume louder.
    #
    # WARNING: The volume hack (see the GEAudioOut.cpp file) is VERY DANGEROUS
    # because the data to access the volume interface is retrieved manually with
    # pointer manipulation. Should the library, in which the interface is
    # implemented, be modified even a tiny bit, the application using this hack
    # might crash.
    #
    #DEFINES += QTGAMEENABLER_USE_VOLUME_HACK    
    
    contains(DEFINES, QTGAMEENABLER_USE_VOLUME_HACK) {
        # Include paths and libraries required for the volume hack.
        message(Symbian volume hack enabled)
        INCLUDEPATH += /epoc32/include/mmf/common
        INCLUDEPATH += /epoc32/include/mmf/server
        LIBS += -lmmfdevsound
    }

    message($$INCLUDEPATH)
    message($$LIBS)
    message($$DEFINES)
    message($$QT)    
}


# Unix based platforms
unix:!symbian {
    # Common
    LIBS += -lX11 -lEGL -lGLESv2
    
    BINDIR = /opt/usr/bin
    DATADIR = /usr/share

    target.path = $$BINDIR
    icon64.files += icons/qtgameenabler.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps            
    
    maemo5 {
        # Maemo 5 specific
        message(Maemo 5 build)
        QT += multimedia

        desktopfile.files += qtc_packaging/debian_fremantle/$${TARGET}.desktop
        desktopfile.path = $$DATADIR/applications/hildon

        INSTALLS += \
            desktopfile \
            icon64
    }
    else {
        contains(DEFINES, DESKTOP) {
            # Unix based desktop specific
            message(Unix based desktop build)
            QT += multimedia

            target.path = /usr/local/bin
            
            INCLUDEPATH += ../SDKPackage_OGLES2/Builds/OGLES2/Include
            LIBS += -L../SDKPackage_OGLES2/Builds/OGLES2/LinuxPC/Lib

            INCLUDEPATH += $(HOME)/Downloads/qt-mobility-opensource-src-1.1.0/install/include
            INCLUDEPATH += $(HOME)/Downloads/qt-mobility-opensource-src-1.1.0/install/include/QtMultimedia
            LIBS += -L$(HOME)/Downloads/qt-mobility-opensource-src-1.1.0/install/lib
        }
        else {
            # Harmattan specific
            message(Harmattan build)
            DEFINES += Q_WS_MAEMO_6

            CONFIG += mobility
            MOBILITY += multimedia

            desktopfile.files += qtc_packaging/debian_harmattan/$${TARGET}.desktop
            desktopfile.path = $$DATADIR/applications

            INSTALLS += \
                desktopfile \
                icon64
        }
    }

    # Common cont.
    INSTALLS += target

    message($$INCLUDEPATH)
    message($$LIBS)
}


windows: {
    message(Windows desktop build)
    QT += multimedia

    TARGET = QtGameEnablerTest

    INCLUDEPATH += /PowerVRSDK/Builds/OGLES2/Include
    LIBS += -L/PowerVRSDK/Builds/OGLES2/WindowsPC/Lib

    message($$INCLUDEPATH)
    message($$LIBS)

    LIBS += -llibEGL -llibGLESv2
}
