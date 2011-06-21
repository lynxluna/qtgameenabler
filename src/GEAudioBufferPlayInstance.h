/**
 * Copyright (c) 2011 Nokia Corporation.
 *
 * Part of the Qt GameEnabler.
 */

#ifndef GEAUDIOBUFFERPLAYINSTANCE_H
#define GEAUDIOBUFFERPLAYINSTANCE_H

#include "GEInterfaces.h"


namespace GE {

// Forward declarations
class AudioBuffer;


class AudioBufferPlayInstance : public AudioSource
{
    Q_OBJECT

public:
    AudioBufferPlayInstance(AudioBuffer *buffer = 0, QObject *parent = 0);
    virtual ~AudioBufferPlayInstance();

public:
    bool isPlaying() const;
    inline bool isFinished() const { return m_finished; }
    inline void setDestroyWhenFinished(bool set) { m_destroyWhenFinished = set; }
    inline bool destroyWhenFinished() const { return m_destroyWhenFinished; }

public: // From AudioSource
    bool canBeDestroyed();
    int pullAudio(AUDIO_SAMPLE_TYPE *target, int bufferLength);

public slots:
    void playBuffer(AudioBuffer *buffer, int loopCount = 0);
    void playBuffer(AudioBuffer *buffer,
                    float volume,
                    float speed,
                    int loopCount = 0);
    void stop();
    void setLoopCount(int count);
    void setSpeed(float speed);
    void setLeftVolume(float volume);
    void setRightVolume(float volume);

protected:
    int mixBlock(AUDIO_SAMPLE_TYPE *target, int bufferLength);

signals:
    void finished();

protected: // Data
    AudioBuffer *m_buffer; // Not owned
    bool m_finished;
    bool m_destroyWhenFinished;
    int m_fixedPos;
    int m_fixedInc;
    int m_fixedLeftVolume;
    int m_fixedRightVolume;
    int m_fixedCenter;
    int m_loopCount;
};

} // namespace GE

#endif // GEAUDIOBUFFERPLAYINSTANCE_H
