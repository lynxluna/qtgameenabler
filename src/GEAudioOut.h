/**
 * Copyright (c) 2011 Nokia Corporation.
 *
 * Part of the Qt GameEnabler.
 */

#ifndef GEAUDIOOUT_H
#define GEAUDIOOUT_H

#include <QThread>
#include "GEInterfaces.h"

// Forward declarations
class QAudioOutput;
class QIODevice;


namespace GE {

class AudioOut : public QThread
{
    Q_OBJECT

public: // Data types

    enum ThreadStates {
        NotRunning = 0,
        DoRun = 1,
        DoExit = 2
    };

public:
    AudioOut(AudioSource *source, QObject *parent = 0);
    virtual ~AudioOut();

public:
    bool usingThead() const { return m_usingThread; }

public slots:
    void tick();

private slots:
    void audioNotify();

protected: // From QThread
     virtual void run(); // For the threaded mode only!

protected: // Data
    QAudioOutput *m_audioOutput; // Owned
    QIODevice *m_outTarget; // Not owned
    AudioSource *m_source; // Not owned
    AUDIO_SAMPLE_TYPE *m_sendBuffer; // Owned
    int m_sendBufferSize;
    qint64 m_samplesMixed;
    int m_threadState;
    bool m_usingThread;
};

} // namespace GE

#endif // GEAUDIOOUT_H
