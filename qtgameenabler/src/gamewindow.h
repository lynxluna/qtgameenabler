/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#ifdef Q_WS_MAEMO_5
#include <QApplication>
#include <QMetaType>
#include <QtCore>
#include <QtGui>
#include <QTextStream>
#endif

#include <QWidget>

#include <EGL/egl.h>
#include "GLES2/gl2.h"

#include "audiomixer.h"
#include "audioout.h"
#include "audiosourceif.h"

#ifdef Q_OS_SYMBIAN
// For volume keys
#include <remconcoreapitargetobserver.h>
#include <remconcoreapitarget.h>
#include <remconinterfaceselector.h>

#include <AccMonitor.h>
#include <w32std.h>
#include <mw/coemain.h>
#include <mw/coedef.h>

class CRepository;
#endif


namespace GE {

class GameWindow : public QWidget
#ifdef Q_OS_SYMBIAN
                  ,public MRemConCoreApiTargetObserver
                  ,public MAccMonitorObserver
#endif
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = 0);
    virtual ~GameWindow();

public:
    void create();
    void destroy();
    bool isProfileSilent() const;
    inline bool audioEnabled() { return m_audioEnabled; }
    AudioMixer &getMixer() { return m_audioMixer; }
    void setHdOutput(bool onOff);
    inline bool hdEnabled() const { return m_hdEnabled; }
    inline bool hdConnected() const { return m_hdConnected; }

public: // Helpers/getters
    unsigned int getTickCount() const;
    float getFrameTime() const { return m_frameTime; }
    float getFPS() const { return m_fps; }
    int width();
    int height();

public slots:
    void pause();
    void resume();
    void startAudio();
    void stopAudio();

protected: // From QWidget (and other base classes)
    bool eventFilter(QObject *object, QEvent *event);
    void paintEvent(QPaintEvent *event) { Q_UNUSED(event); } // Overriden
    virtual void resizeEvent(QResizeEvent *event);
    void timerEvent(QTimerEvent *event);

protected: // Application callbacks, override these in your own derived class
    virtual int onCreate();
    virtual void onInitEGL();
    virtual void onFreeEGL();
    virtual void onDestroy();
    virtual void onRender();
    virtual void onPause();
    virtual void onResume();
    virtual void onVolumeUp();
    virtual void onVolumeDown();

    virtual void update(const float frameDelta);
    virtual void setSize(int width, int height);

protected: // For internal functionality
    virtual void createEGL();
    void reinitEGL();
    void render();
    bool testEGLError(const char* pszLocation);
    void cleanupAndExit(EGLDisplay eglDisplay);
    virtual EGLNativeWindowType getWindow();

#ifdef Q_OS_SYMBIAN
protected:
    void DestroyWindow();
    void CreateWindowL();

protected:
    // For volume keys
    void MrccatoCommand(TRemConCoreApiOperationId aOperationId,
        TRemConCoreApiButtonAction aButtonAct);

    // MAccMonitorObserver
    virtual void ConnectedL(CAccMonitorInfo *aAccessoryInfo);
    virtual void DisconnectedL(CAccMonitorInfo *aAccessoryInfo);
    virtual void AccMonitorObserverError(TInt aError) { }

#endif

protected: // Data
    // EGL variables
    EGLDisplay eglDisplay;
    EGLConfig eglConfig;
    EGLSurface eglSurface;
    EGLContext eglContext;

    // Time calculation
    unsigned int m_prevTime;
    unsigned int m_currentTime;
    float m_frameTime;
    float m_fps;
    bool m_paused;
    int m_timerId;

    // Audio
    AudioOut *m_audioOutput;
    AudioMixer m_audioMixer;
    bool m_audioEnabled;

    // HD output
    bool m_hdEnabled;
    bool m_hdConnected;

#ifdef Q_OS_SYMBIAN

    // TODO: Move HD and volume control into separate classes

    // For HD output
    RWsSession iWsSession;
    CWsScreenDevice *iScreenDevice;
    RWindowGroup *iWindowGroup;
    RWindow *iWindow;
    CAccMonitor *iAccMonitor;

    // For volume keys and active profile
    CRemConInterfaceSelector *iInterfaceSelector;
    CRemConCoreApiTarget *iCoreTarget;
    CRepository *iProfileRepository;

#endif
};

} // namespace GE

#endif // GAMEWINDOW_H
