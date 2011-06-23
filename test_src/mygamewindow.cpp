/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#include "mygamewindow.h"


/*!
  \class MyGameWindow
  \brief TODO
*/


/*!
  Constructor.
*/
MyGameWindow::MyGameWindow(QWidget *parent /* = 0 */)
    : GE::GameWindow(parent)
{
}


/*!
  Destructor.
*/
MyGameWindow::~MyGameWindow()
{
}


/*!
  From GameWindow.

  \see GameWindow.cpp
*/
void MyGameWindow::onRender()
{
    // Use OpenGL freely.
    static float val(0.0f);
    val += 0.01f;

    if (val > 1.0f) {
        val -= 1.0f;
    }

    glClearColor(0.0f, val, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
