/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#include <QKeyEvent>
#include "mygamewindoweventfilter.h"


/*!
  \class KeyPressEater
  \brief An event filter for capturing key press events.
*/


/*!
  Filters \a event sent or posted by \a obj. In order for this event filter
  instance to receive the events, it has to be installed for \a obj first.
*/
bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        qDebug("Ate key press %d", keyEvent->key());
        return true;
    }

    // Let the event propagate for standard event processing.
    return QObject::eventFilter(obj, event);
}
