/**
 * Copyright (c) 2011 Nokia Corporation.
 */

#ifndef TRACE_H
#define TRACE_H

#ifdef GE_DEBUG
    #include <QDebug>

    #define DEBUG_POINT qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__
    #define DEBUG_INFO(ARG...) \
        qDebug() << __PRETTY_FUNCTION__ << ":" << __LINE__ << ":" << ARG
#else
    #define DEBUG_POINT do {} while (0)
    #define DEBUG_INFO(...) do {} while (0)
#endif

#endif // TRACE_H

// End of file.
