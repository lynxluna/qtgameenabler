/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#ifndef __GAMEENABLEREXAMPLEMAIN__
#define __GAMEENABLEREXAMPLEMAIN__

#include <QtGui/QApplication>
#include <QPaintEngine>

#ifdef Q_OS_SYMBIAN
    #include <eikenv.h>
    #include <eikappui.h>
    #include <aknenv.h>
    #include <aknappui.h>
#endif

#include "mygamewindow.h"
#include "mygamewindoweventfilter.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_SYMBIAN
    // Lock orientation to landscape
    CAknAppUi* appUi = dynamic_cast<CAknAppUi*>(CEikonEnv::Static()->AppUi());
    TRAPD(error,
        if (appUi) {
            appUi->SetOrientationL(CAknAppUi::EAppUiOrientationLandscape);
        }
    );
#endif

    GE::GameWindow* theGameWindow = new MyGameWindow();
    theGameWindow->create();
    theGameWindow->setWindowState(Qt::WindowNoState);

    qApp->installEventFilter(theGameWindow);

#ifdef Q_OS_WIN32
    theGameWindow->showMaximized();
#else
    theGameWindow->showFullScreen();
#endif

    int result = a.exec();
    theGameWindow->destroy();
    delete theGameWindow;
    return result;
}


#endif /* __GAMEENABLEREXAMPLEMAIN__ */
