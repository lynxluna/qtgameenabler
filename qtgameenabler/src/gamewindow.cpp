/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#include "gamewindow.h"

#include <QtGui>

#ifdef Q_OS_LINUX
#include <QX11Info>
#endif

#include <GLES2/gl2.h>

#ifdef Q_WS_MAEMO_6
#include <qmeegographicssystemhelper.h>
#endif

#ifdef Q_OS_SYMBIAN
#include <mw/coecntrl.h>
#include <centralrepository.h>

// Uid for Profile repository
const TUid KCRUidProfileEngine =
        {
        0x101F8798
        };

// Key for warning and game tones enabled in active profile
const TUint32 KProEngActiveWarningTones = 0x7E000020;

_LIT(KWgName, "HdmiOutput");
const TUint32 KWgId = 0xeeeeeeee; // TODO: define unique id
const TUint32 KHdScreen = 1;

#endif

#include "trace.h" // For debug macros

using namespace GE;


/*!
  \class GameWindow
  \brief QtWidget with native OpenGL ES 2.0 support. Replaces QGLWidget when
         real OpenGL ES is required instead of QtOpenGL.
*/


/*!
  Constructor.
*/
GameWindow::GameWindow(QWidget *parent /* = 0 */)
    : QWidget(parent),
      m_prevTime(0),
      m_currentTime(0),
      m_frameTime(0.0f),
      m_fps(0.0f),
      m_paused(true),
      m_timerId(0),
      m_audioOutput(0),
      m_audioEnabled(false),
      m_hdEnabled(false),
      m_hdConnected(false)
#ifdef Q_OS_SYMBIAN
    , iScreenDevice(NULL),
      iWindowGroup(NULL),
      iWindow(NULL),
      iAccMonitor(NULL),
      iInterfaceSelector(NULL),
      iProfileRepository(NULL)
#endif
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_PaintUnclipped);

#ifdef GE_USE_MM_KEYS
    QApplication::setAttribute(Qt::AA_CaptureMultimediaKeys);
#endif

#ifdef Q_WS_MAEMO_6
    QMeeGoGraphicsSystemHelper::setSwitchPolicy(QMeeGoGraphicsSystemHelper::NoSwitch);
#endif

#ifdef Q_OS_SYMBIAN
    // For volume keys
    TRAP_IGNORE(
        iInterfaceSelector = CRemConInterfaceSelector::NewL();
        iCoreTarget = CRemConCoreApiTarget::NewL(*iInterfaceSelector, *this);
        iInterfaceSelector->OpenTargetL();
    );

    TRAP_IGNORE(
        iProfileRepository = CRepository::NewL( KCRUidProfileEngine );
    );
#endif
}


/*!
  Destructor.
*/
GameWindow::~GameWindow()
{
#ifdef Q_OS_SYMBIAN
    delete iProfileRepository;
    iProfileRepository = NULL;

    // For volume keys
    delete iInterfaceSelector;
    iInterfaceSelector = NULL;
    iCoreTarget = NULL; // owned by interfaceselector

    DestroyWindow();

    if (iAccMonitor) {
        delete iAccMonitor;
        iAccMonitor = NULL;

        iWsSession.Close();
    }
#endif

    stopAudio();
    destroy();
}


/*!
  Initializes OpenGL.
*/
void GameWindow::create()
{
    DEBUG_POINT;
    setAttribute(Qt::WA_NoSystemBackground);
    createEGL();

    onCreate();
    onInitEGL();

    m_currentTime = getTickCount();
    m_prevTime = m_currentTime;
    m_fps = 0.0f;
    m_frameTime = 0.0f;

    resume();

    DEBUG_INFO("Finished!");
}


/*!
  Called when the game window is to be destroyed.
*/
void GameWindow::destroy()
{
    DEBUG_POINT;
    onFreeEGL();
    onDestroy();
}


/*!
  Returns true if the current profile is silent.
*/
bool GameWindow::isProfileSilent() const
{
#ifdef Q_OS_SYMBIAN
    TInt warningTonesEnabled(1);

    if ( iProfileRepository &&
         !iProfileRepository->Get( KProEngActiveWarningTones, warningTonesEnabled ) &&
         !warningTonesEnabled )
        {
        return true;
        }
#endif

    return false;
}


/*!
  Returns the tick count in milliseconds.
*/
unsigned int GameWindow::getTickCount() const
{
    QTime now = QTime::currentTime();
    return (now.hour() * 3600 + now.minute() * 60 + now.second()) * 1000
            + now.msec();
}


/*!
  Pauses the execution.
*/
void GameWindow::pause()
{
    DEBUG_POINT;
    m_paused = true;
    stopAudio();
    killTimer(m_timerId);
    m_timerId = 0;
    onPause();
}


/*!
  Resumes from the pause mode.
*/
void GameWindow::resume()
{
    DEBUG_POINT;

    if (m_timerId)
        return;

#ifdef Q_OS_SYMBIAN
    // Wait a little while to be able to see the new profile correctly.
    User::After( 500000 );
#endif

    m_paused = false;

    if (!isProfileSilent())
        startAudio();

    onResume();
    m_timerId = startTimer(0);
}


/*!
  Starts the audio if not started.
*/
void GameWindow::startAudio()
{
    if (isProfileSilent())
        return;

    // Already enabled
    if (m_audioOutput != 0)
        return;

    m_audioEnabled = true;
    DEBUG_INFO("Starting audio..");
    m_audioOutput = new GE::AudioOut(&m_audioMixer, this);
}


/*!
  Stops the audio.
*/
void GameWindow::stopAudio()
{
    // Already stopped.
    if (m_audioOutput == 0)
        return;

    m_audioEnabled = false;
    delete m_audioOutput;
    m_audioOutput = 0;
}

/**
  Return native window handle
*/
EGLNativeWindowType GameWindow::getWindow()
{
    EGLNativeWindowType pwd;

#ifdef Q_OS_SYMBIAN
    if (iWindow)
        pwd = (EGLNativeWindowType)iWindow;
    else
        pwd = (EGLNativeWindowType)(this->winId()->DrawableWindow());
#else
    pwd = (EGLNativeWindowType)this->winId();
#endif

    return pwd;
}

void GameWindow::setHdOutput(bool onOff)
{
#ifdef Q_OS_SYMBIAN
    if (onOff) {
        if (!iAccMonitor) {
            QT_TRAP_THROWING(
                User::LeaveIfError(iWsSession.Connect());

                iAccMonitor = CAccMonitor::NewL();

                // invoke callback for all currently connected accessories
                RConnectedAccessories array;
                CleanupClosePushL(array);
                iAccMonitor->GetConnectedAccessoriesL(array);

                for (int i = 0; i < array.Count(); i++)
                    ConnectedL(array[i]);

                CleanupStack::PopAndDestroy(&array);

                // register to listen for changes
                RAccMonCapabilityArray capabilityArray;
                CleanupClosePushL(capabilityArray);
                capabilityArray.Append(KAccMonHDMI);
                iAccMonitor->StartObservingL(this, capabilityArray);
                CleanupStack::PopAndDestroy(&capabilityArray);
            );
        }
    }
    else {
        if (m_hdConnected) {
            m_hdConnected = false;
            reinitEGL();
        }
        if (iAccMonitor) {
            iAccMonitor->StopObserving();
            delete iAccMonitor;
            iAccMonitor = NULL;

            iWsSession.Close();
        }
    }
#endif
    m_hdEnabled = onOff;
}


/*!
  From QObject.

  Filters \a event sent or posted by \a object. If the event is
  ActivationChanged event, the pause mode is toggled. Otherwise the event is
  propagated.
*/
bool GameWindow::eventFilter(QObject *object, QEvent *event)
{
    // http://doc.trolltech.com/4.7/qevent.html#QEvent
    if (event->type() == QEvent::ActivationChange) {
        if (m_paused)
            resume();
        else
            pause();

        DEBUG_INFO("ActivationChange event filtered -> toggle pause");
        return false;
    }
#ifdef GE_USE_MM_KEYS
    else if (event->type () == QEvent::KeyPress) {
        QKeyEvent* k = (QKeyEvent*)event;
        switch (k->key()) {
            case Qt::Key_VolumeDown:
                onVolumeDown();
                return true;
            case Qt::Key_VolumeUp:
                onVolumeUp();
                return true;
            default:
                break;
        }
    }
#endif

    // Let the event propagate for standard event processing.
    return QObject::eventFilter(object, event);
}

int GameWindow::width()
{
#ifdef Q_OS_SYMBIAN
    if (iWindow)
        return iScreenDevice->SizeInPixels().iWidth;
#endif

    return QWidget::width();
}

int GameWindow::height()
{
#ifdef Q_OS_SYMBIAN
    if (iWindow)
        return iScreenDevice->SizeInPixels().iHeight;
#endif

    return QWidget::height();
}

/*!
  From QWidget.

  Handles the resize events.
*/
void GameWindow::resizeEvent(QResizeEvent *event)
{
    int w, h;
    if (m_hdConnected) {
        // always fake hdmi output resolution
        w = width();
        h = height();
    }
    else {
        if (event->size().width() == event->oldSize().width()
                && event->size().height() == event->oldSize().height()) {
            // The size has not changed.
            return;
        }

        w = event->size().width();
        h = event->size().height();
    }

    DEBUG_INFO("Setting the viewport.");
    glViewport(0,0, w, h);
    setSize(w, h);

    DEBUG_INFO("New size:" << w << "," << h);

    // Call the base class' resizeEvent().
    QWidget::resizeEvent(event);
}


/*!
  From QObject.

  Handles the timer events.
*/
void GameWindow::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event); // To prevent compiler warnings.
    render();
}

/*!
  Called after OpenGL ES 2.0 is created to let the application to allocate
  its resources.

  To be implemented in the derived class.
*/
int GameWindow::onCreate()
{
    DEBUG_POINT;
    return 1;
}

void GameWindow::onInitEGL()
{
    DEBUG_POINT;
}

void GameWindow::onFreeEGL()
{
    DEBUG_POINT;
}


/*!
  Called when everything is about to be destroyed and the application is
  shutting down.

  To be implemented in the derived class.
*/
void GameWindow::onDestroy()
{
    DEBUG_POINT;
}


/*!
  Called when application needs to (re)render its screen.

  To be implemented in the derived class.
*/
void GameWindow::onRender()
{
    DEBUG_POINT;
}


/*!
  Called when the framework is going into pause mode.

  To be implemented in the derived class.
*/
void GameWindow::onPause()
{
    DEBUG_POINT;
}


/*!
  Called when the framework is resuming from the pause mode.

  To be implemented in the derived class.
*/
void GameWindow::onResume()
{
    DEBUG_POINT;
}


/*!
  Called when volume up -key is pressed.

  Default implementation increases absolute volume by 1/20.
*/
void GameWindow::onVolumeUp()
{
    DEBUG_POINT;
    m_audioMixer.setAbsoluteVolume(
        qMin(1.0f, m_audioMixer.absoluteVolume() + 1.0f / 20.0f));
}


/*!
  Called when volume down -key is pressed.

  Default implementation decreases absolute volume by 1/20.
*/
void GameWindow::onVolumeDown()
{
    DEBUG_POINT;
    m_audioMixer.setAbsoluteVolume(
        qMax(0.0f, m_audioMixer.absoluteVolume() - 1.0f / 20.0f));
}

/*!
  Called once before each frame, \a frameDelta attribute is set as the time
  between current frame and the previous one.

  To be implemented in the derived class.
*/
void GameWindow::update(const float frameDelta)
{
    Q_UNUSED(frameDelta);
}


/*!
  Called when the size of the screen has been changed. The application could
  update its projection, viewport and other size specific stuff here.

  To be implemented in the derived class.
*/
void GameWindow::setSize(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}


/*!
  Create and initialize objects required for OpenGL rendering
*/
void GameWindow::createEGL()
{
    // EGL variables
    eglDisplay	= 0;
    eglConfig	= 0;
    eglSurface	= 0;
    eglContext	= 0;

#ifdef Q_OS_LINUX
    eglDisplay =
        eglGetDisplay((EGLNativeDisplayType)this->x11Info().display());
#endif

#ifdef Q_OS_WIN32
    HWND hwnd = this->winId();
    HDC dc = GetWindowDC(hwnd);
    eglDisplay = eglGetDisplay((EGLNativeDisplayType)dc);
#endif

#ifdef Q_OS_SYMBIAN
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif

    DEBUG_INFO("eglGetDisplay ==" << eglDisplay);

    EGLint majorVersion;
    EGLint minorVersion;

    if (!eglInitialize(eglDisplay, &majorVersion, &minorVersion)) {
        DEBUG_INFO("Error: eglInitialize() failed!");
        cleanupAndExit(eglDisplay);
    }

    DEBUG_INFO("eglInitialize() finished");

    EGLint pi32ConfigAttribs[13];
    pi32ConfigAttribs[0] = EGL_SURFACE_TYPE;
    pi32ConfigAttribs[1] = EGL_WINDOW_BIT;
    pi32ConfigAttribs[2] = EGL_RENDERABLE_TYPE;
    pi32ConfigAttribs[3] = EGL_OPENGL_ES2_BIT;

    pi32ConfigAttribs[4] = EGL_DEPTH_SIZE;
    pi32ConfigAttribs[5] = 8;
    pi32ConfigAttribs[6] = EGL_NONE;

    EGLint pi32ContextAttribs[3];
    pi32ContextAttribs[0] = EGL_CONTEXT_CLIENT_VERSION;
    pi32ContextAttribs[1] = 2;
    pi32ContextAttribs[2] = EGL_NONE;

    EGLint configs;

    if (!eglChooseConfig(eglDisplay, pi32ConfigAttribs, &eglConfig, 1, &configs)
            || (configs != 1))  {
        DEBUG_INFO("Error: eglChooseConfig() failed!");
        cleanupAndExit(eglDisplay);
    }

    DEBUG_INFO("eglChooseConfig() finished");

    eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, getWindow(), NULL);
    DEBUG_INFO("englCreateWindowSurface() finished");

    if (!testEGLError("eglCreateWindowSurface")) {
        cleanupAndExit(eglDisplay);
    }

    eglContext = eglCreateContext(eglDisplay, eglConfig, NULL,
                                  pi32ContextAttribs);
    DEBUG_INFO("eglCreateContext() finished");

    if (!testEGLError("eglCreateContext")) {
        cleanupAndExit(eglDisplay);
    }

    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    DEBUG_INFO("eglMakeCurrent() finished");

    if (!testEGLError("eglMakeCurrent")) {
        cleanupAndExit(eglDisplay);
    }
}


/*!
  Main run - method for the QtGameEnabler, processes a single frame and
  calls necesseary functions of the application using QtGE: update and
  onRender.
*/
void GameWindow::render()
{
    m_prevTime = m_currentTime;
    m_currentTime = getTickCount();
    m_frameTime = (float)(m_currentTime - m_prevTime) * 0.001f;

    if (m_frameTime != 0.0f)
        m_fps = 1.0f / m_frameTime;
    else
        m_fps = 100000.0f;

    if (m_audioOutput && m_audioOutput->usingThead() == false)
        m_audioOutput->tick(); // Manual tick

    update(m_frameTime);

    if (m_audioOutput && m_audioOutput->usingThead() == false)
        m_audioOutput->tick(); // Manual tick

    onRender();

    if (!eglSwapBuffers(eglDisplay, eglSurface)) {
        // eglSwapBuffers() failed!
        GLint errVal = eglGetError();

        /*
        EXPLANATION:

        http://library.forum.nokia.com/index.jsp?topic=/Nokia_Symbian3_Developers_Library/GUID-894AB487-C127-532D-852B-37CB0DEA1440.html

        On the Symbian platform, EGL handles the window resize in the next
        call to eglSwapBuffers(), which resizes the surface to match the new
        window size. If the preserve buffer option is in use, this function
        also copies across all the pixels from the old surface that overlap
        the new surface, although the exact details depend on the
        implementation.

        If the surface resize fails, eglSwapBuffers() returns EGL_FALSE and an
        EGL_BAD_ALLOC error is raised. This may mean that the implementation
        does not support the resizing of a surface or that there is not enough
        memory available (on a platform with GPU, this would be GPU rather
        than system memory).

        Applications must always monitor whether eglSwapBuffers() fails after
        a window resize. When it does fail, the application should do the
        following:
          - Call eglDestroySurface() to destroy the current EGL window surface.
          - Call eglCreateWindowSurface() to recreate the EGL window surface.

        This may cause a noticeable flicker and so should be done only when
        necessary.
        */

        DEBUG_INFO("eglSwapbuffers() failed with error:" << errVal);

        if (errVal == EGL_BAD_ALLOC || errVal == EGL_BAD_SURFACE) {
            if (errVal == EGL_BAD_ALLOC)
                DEBUG_INFO("Error was bad alloc, taking care of it.");

            eglDestroySurface(eglDisplay, eglSurface);

            eglSurface = eglCreateWindowSurface(eglDisplay,
                                                eglConfig,
                                                getWindow(),
                                                NULL);
            eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
        }
        else {
            cleanupAndExit(eglDisplay);
        }
    }
}


/*!
  Check for EGL errors. \a pszLocation should contain the last called EGL
  function and is used for DEBUG_INFO print. The method returns true in case of no
  errors, false otherwise.
*/
bool GameWindow::testEGLError(const char *pszLocation)
{
    EGLint err = eglGetError();

    if (err != EGL_SUCCESS) {
        DEBUG_INFO(pszLocation << "failed" << err);
        return false;
    }

    return true;
}


/*!
  Cleans up the content of \a eglDisplay and exits.
*/
void GameWindow::cleanupAndExit(EGLDisplay eglDisplay)
{
    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(eglDisplay, eglSurface);
#ifdef Q_OS_SYMBIAN
    DestroyWindow();
#endif
    eglTerminate(eglDisplay);
    exit(0);
}

void GameWindow::reinitEGL()
{
    onFreeEGL();

    eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(eglDisplay, eglSurface);

#ifdef Q_OS_SYMBIAN
    DestroyWindow();

    if (m_hdConnected)
        QT_TRAP_THROWING(CreateWindowL());
#endif

    createEGL();
    onInitEGL();
}

#ifdef Q_OS_SYMBIAN
void GameWindow::ConnectedL(CAccMonitorInfo *aAccessoryInfo)
{
    if (aAccessoryInfo->Exists(KAccMonHDMI)) {
        m_hdConnected = true;
        reinitEGL();
    }
}

void GameWindow::DisconnectedL(CAccMonitorInfo *aAccessoryInfo)
{
    if (aAccessoryInfo->Exists(KAccMonHDMI)) {
        m_hdConnected = false;
        reinitEGL();
    }
}

void GameWindow::CreateWindowL()
{
    if (iWindow)
        return;

    iScreenDevice = new (ELeave) CWsScreenDevice(iWsSession);
    TInt err = iScreenDevice->Construct(KHdScreen);
    User::LeaveIfError(err);

    iWsSession.ComputeMode(RWsSession::EPriorityControlDisabled);

    iWindowGroup = new (ELeave) RWindowGroup(iWsSession);
    err = iWindowGroup->Construct(KWgId, iScreenDevice);
    User::LeaveIfError(err);
    err = iWindowGroup->SetName(KWgName);
    User::LeaveIfError(err);

    iWindowGroup->EnableReceiptOfFocus(EFalse);

    TSize size = iScreenDevice->SizeInPixels();

    iWindow = new (ELeave) RWindow(iWsSession);
    User::LeaveIfError(iWindow->Construct(*iWindowGroup, (TUint32) iWindow));

    iWindow->SetExtent(TPoint(0, 0), size);
    iWindow->SetOrdinalPosition(0, ECoeWinPriorityAlwaysAtFront + 1);
    iWindow->SetNonFading(ETrue);
    iWindow->SetVisible(ETrue);
    iWindow->Activate();
}

void GameWindow::DestroyWindow()
{
    if (iWindow) {
        iWindow->Close();
        delete iWindow;
        iWindow = NULL;
    }

    if (iWindowGroup) {
        iWindowGroup->Close();
        delete iWindowGroup;
        iWindowGroup = NULL;
    }

    delete iScreenDevice;
    iScreenDevice = NULL;
}

// For volume keys
void GameWindow::MrccatoCommand(TRemConCoreApiOperationId aOperationId,
                                TRemConCoreApiButtonAction aButtonAct)
{
    //if (aButtonAct == ERemConCoreApiButtonClick) {
    if (aButtonAct == ERemConCoreApiButtonPress) {
        if (aOperationId == ERemConCoreApiVolumeUp)
            onVolumeUp();
        else if (aOperationId == ERemConCoreApiVolumeDown)
            onVolumeDown();
    }
}

#endif

