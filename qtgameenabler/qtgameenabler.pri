# Copyright (c) 2011 Nokia Corporation.

QT += core gui declarative

# Comment the following line out for better performance. Using the definition
# enables debug logging which is convenient for locating problems in the code
# but is also very costly in terms of performance.
#DEFINES += GE_DEBUG

INCLUDEPATH += $${GE_PATH}/src

HEADERS  += \
    $${GE_PATH}/src/audiobuffer.h \
    $${GE_PATH}/src/audiobufferplayinstance.h \
    $${GE_PATH}/src/audiomixer.h \
    $${GE_PATH}/src/audioout.h \
    $${GE_PATH}/src/audiosourceif.h \
    $${GE_PATH}/src/gamewindow.h \
    $${GE_PATH}/src/trace.h

SOURCES += \
    $${GE_PATH}/src/audiobuffer.cpp \
    $${GE_PATH}/src/audiobufferplayinstance.cpp \
    $${GE_PATH}/src/audiomixer.cpp \
    $${GE_PATH}/src/audioout.cpp \
    $${GE_PATH}/src/audiosourceif.cpp \
    $${GE_PATH}/src/gamewindow.cpp


symbian {
    message(Symbian build)

    CONFIG += mobility
    MOBILITY += multimedia

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
}


# Unix based platforms
unix:!symbian {
    # Common
    LIBS += -lX11 -lEGL -lGLESv2

    maemo5 {
        # Maemo 5 specific
        message(Maemo 5 build)
        QT += multimedia
    }
    else {
        contains(DEFINES, DESKTOP) {
            # Unix based desktop specific
            message(Unix based desktop build)
            QT += multimedia

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
            QT += meegographicssystemhelper
        }
    }
}


windows: {
    message(Windows desktop build)
    QT += multimedia

    TARGET = QtGameEnablerTest

    INCLUDEPATH += /PowerVRSDK/Builds/OGLES2/Include
    LIBS += -L/PowerVRSDK/Builds/OGLES2/WindowsPC/Lib

    LIBS += -llibEGL -llibGLESv2
}


message($$INCLUDEPATH)
message($$LIBS)

# End of file.
