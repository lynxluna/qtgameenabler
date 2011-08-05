# Copyright (c) 2011 Nokia Corporation.

QT += core gui

TARGET = qtgameenablertest
TEMPLATE = app
VERSION = 1.1.1

GE_PATH = ../qtgameenabler
include(../qtgameenabler/qtgameenabler.pri)

INCLUDEPATH += ./test_src

SOURCES += \
    test_src/main.cpp \
    test_src/mygamewindow.cpp \
    test_src/mygamewindoweventfilter.cpp

HEADERS  += \
    test_src/mygamewindow.h \
    test_src/mygamewindoweventfilter.h


symbian {
    TARGET = QtGameEnablerTest
    TARGET.UID3 = 0xee69dad8
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000

    ICON = $${GE_PATH}/icons/qtgameenabler.svg
}


# Unix based platforms
unix:!symbian {
    # Common
    BINDIR = /opt/usr/bin
    DATADIR = /usr/share

    target.path = $$BINDIR
    icon64.files += $${GE_PATH}/icons/qtgameenabler.png
    icon64.path = $$DATADIR/icons/hicolor/64x64/apps            
    
    maemo5 {
        # Maemo 5 specific
        desktopfile.files += qtc_packaging/debian_fremantle/$${TARGET}.desktop
        desktopfile.path = $$DATADIR/applications/hildon

        INSTALLS += \
            desktopfile \
            icon64
    }
    else {
        contains(DEFINES, DESKTOP) {
            # Unix based desktop specific
            target.path = /usr/local/bin
        }
        else {
            # Harmattan specific
            desktopfile.files += qtc_packaging/debian_harmattan/$${TARGET}.desktop
            desktopfile.path = $$DATADIR/applications

            INSTALLS += \
                desktopfile \
                icon64
        }
    }

    # Common cont.
    INSTALLS += target
}


windows {
    TARGET = QtGameEnablerTest
}

# End of file.
