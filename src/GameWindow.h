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

#include "GEAudioMixer.h"
#include "GEAudioOut.h"
#include "GEInterfaces.h"

#ifdef Q_OS_SYMBIAN
class CRepository;
#endif


namespace GE {

class GameWindow : public QWidget
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

public: // Helpers/getters
    unsigned int getTickCount() const;
    float getFrameTime() const { return m_frameTime; }
    float getFPS() const { return m_fps; }

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
    virtual void onDestroy();
    virtual void onRender();
    virtual void onPause();
    virtual void onResume();
    virtual void update(const float frameDelta);
    virtual void setSize(int width, int height);

protected: // For internal functionality
    virtual void createEGL();
    void render();
    bool testEGLError(const char* pszLocation);
    void cleanupAndExit(EGLDisplay eglDisplay);

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

#ifdef Q_OS_SYMBIAN
    CRepository *iProfileRepository; // Owned
#endif
};

} // namespace GE

#endif // GAMEWINDOW_H
