/**
 * Copyright (c) 2011 Nokia Corporation.
 *
 * Part of the Qt GameEnabler.
 */

#ifndef GEAUDIOMIXER_H
#define GEAUDIOMIXER_H

#include <QMutex>
#include "GEInterfaces.h"


namespace GE {

class AudioMixer : public AudioSource
{
    Q_OBJECT

public:
    explicit AudioMixer(QObject *parent = 0);
    virtual ~AudioMixer();

public:
    float absoluteVolume() const;
    float generalVolume();
    bool addAudioSource(AudioSource *source);
    bool removeAudioSource(AudioSource *source);
    void destroyList();
    int audioSourceCount();

public: // From AudioSource
    int pullAudio(AUDIO_SAMPLE_TYPE *target, int bufferLength);

public slots:
    void setAbsoluteVolume(float volume);
    void setGeneralVolume(float volume);

signals:
    void absoluteVolumeChanged(float volume);
    void generalVolumeChanged(float volume);

protected: // Data
    QList<AudioSource*> m_sourceList; // Owned
    AUDIO_SAMPLE_TYPE *m_mixingBuffer; // Owned
    QMutex m_mutex;
    int m_mixingBufferLength;
    int m_fixedGeneralVolume;
};

} // namespace GE

#endif // GEAUDIOMIXER_H
