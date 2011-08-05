/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#ifndef MYGAMEWINDOW_H
#define MYGAMEWINDOW_H

#include "gamewindow.h"


class MyGameWindow : public GE::GameWindow
{
    Q_OBJECT

public:
    explicit MyGameWindow(QWidget *parent = 0);
    virtual ~MyGameWindow();

protected: // From GameWindow
    void onRender();
};


#endif // MYGAMEWINDOW_H
