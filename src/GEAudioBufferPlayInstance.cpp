/**
 * Copyright (c) 2011 Nokia Corporation.
 *
 * Part of the Qt GameEnabler.
 */

#include "GEAudioBufferPlayInstance.h"
#include "GEAudioBuffer.h"
#include "trace.h"

using namespace GE;

// Constants
const float GEMaxAudioSpeedValue(4096.0f);
const float GEDefaultAudioVolume(1.0f); // 1.0 => 100 %
const float GEDefaultAudioSpeed(1.0f); // 1.0 => 100 %


/*!
 * \class AudioBufferPlayInstance
 * \brief An AudioSource instance capable of playing a single audio buffer.
 */


/*!
  Constructor. If \a buffer is not NULL, it is set as the buffer to play.
*/
AudioBufferPlayInstance::AudioBufferPlayInstance(AudioBuffer *buffer /* = 0 */,
                                                 QObject *parent /* = 0 */)
    : AudioSource(parent),
      m_buffer(0),
      m_finished(false),
      m_destroyWhenFinished(true),
      m_fixedPos(0),
      m_fixedInc(0),
      m_fixedLeftVolume((int)GEMaxAudioVolumeValue),
      m_fixedRightVolume((int)GEMaxAudioVolumeValue),
      m_fixedCenter(0),
      m_loopCount(0)
{
    if (buffer) {
        // Start playing the given buffer.
        playBuffer(buffer, GEDefaultAudioVolume, GEDefaultAudioSpeed);
    }
}


/*!
  Destructor.
*/
AudioBufferPlayInstance::~AudioBufferPlayInstance()
{
}


/*!
  Returns true if the buffer is set, false otherwise.
*/
bool AudioBufferPlayInstance::isPlaying() const
{
    if (m_buffer)
        return true;

    return false;
}


/*!
  From AudioSource.

  The framework will use this to know whether this AudioSource can be
  destroyed or not.
*/
bool AudioBufferPlayInstance::canBeDestroyed()
{
    if (m_finished && m_destroyWhenFinished)
        return true;

    return false;
}


/*!
  From AudioSource.

  Returns an audio stream from the current sample.
*/
int AudioBufferPlayInstance::pullAudio(AUDIO_SAMPLE_TYPE *target,
                                       int bufferLength)
{
    if (!m_buffer) {
        // No sample!
        return 0;
    }

    int divider(m_buffer->getNofChannels() * m_buffer->getBytesPerSample());
    int channelLength(0);

    // Check in case of division by zero.
    if (divider) {
        channelLength = m_buffer->getDataLength() / divider - 2;
    }
    else {
        DEBUG_INFO("Warning: Catched division by zero error!");
    }

    int samplesToWrite(bufferLength / 2);
    int amount(0);
    int totalMixed(0);


    while (samplesToWrite > 0) {
        int samplesLeft = channelLength - (m_fixedPos >> 12);

        if (m_fixedInc == 0) {
            // No speed set. Will lead to division by zero error if not set.
            setSpeed(GEDefaultAudioSpeed);
        }

        // This is how much we can mix at least.
        int maxMixAmount = (int)(((long long int)(samplesLeft) << 12) /
                                 m_fixedInc);

        if (maxMixAmount > samplesToWrite) {
            maxMixAmount = samplesToWrite;
        }

        if (maxMixAmount > 0) {
            amount = mixBlock(target+totalMixed * 2, maxMixAmount);

            if (amount == 0) {
                // Error!
                break;
            }

            totalMixed += amount;
        }
        else {
            amount = 0;
            m_fixedPos = channelLength<<12;
        }

        // The sample ended. Check the looping variables and see what to do.
        if ((m_fixedPos >> 12) >= channelLength) {
            m_fixedPos -= (channelLength << 12);

            if (m_loopCount > 0)
                m_loopCount--;

            if (m_loopCount == 0) {
                // No more loops, stop the sample and return the amount of
                // samples already mixed.
                stop();
                return totalMixed;
            }
        }

        samplesToWrite -= amount;

        if (samplesToWrite < 1)
            break;
    }

    return totalMixed * 2;
}


/*!
  Sets \a buffer as the audio buffer and will repeat the buffer according to
  \a loopCount. Note: If the given loop count is -1, the buffer will be
  repeated forever.
*/
void AudioBufferPlayInstance::playBuffer(AudioBuffer *buffer,
                                         int loopCount /* = 0 */)
{
    m_buffer = buffer;
    m_loopCount = loopCount;
    m_fixedPos = 0;
}


/*!
  For convenience.

  In addition to playBuffer(AudioBuffer*, int) method, will also set \a volume
  and \a speed.
*/
void AudioBufferPlayInstance::playBuffer(AudioBuffer *buffer,
                                         float volume,
                                         float speed,
                                         int loopCount /* = 0 */)
{
    setLeftVolume(volume);
    m_fixedRightVolume = m_fixedLeftVolume;
    setSpeed(speed);
    playBuffer(buffer, loopCount);
}


/*!
  Resets the local buffer i.e. gets rid of the set buffer.
*/
void AudioBufferPlayInstance::stop()
{
    m_buffer = 0;
    m_finished = true;
    emit finished();
}


/*!
  Sets the loop count to \a count. If the argument value is -1, the
  buffer is looped forever.
*/
void AudioBufferPlayInstance::setLoopCount(int count)
{
    DEBUG_INFO("Setting the loop count to " << count);
    m_loopCount = count;
}


/*!
  Sets \a speed as the speed of which the buffer is played in. The given
  argument value should be between 0.0 and 1.0 since 1.0 indicates 100 %.
*/
void AudioBufferPlayInstance::setSpeed(float speed)
{
    if (!m_buffer)
        return;

    m_fixedInc =
        (int)(((float)m_buffer->getSamplesPerSec() *
               GEMaxAudioSpeedValue * speed) /
              (float)AUDIO_FREQUENCY);
}


/*!
  Sets \a volume for the left channel. The given argument value should be
  between 0.0 and 1.0 since 1.0 indicates 100 %.
*/
void AudioBufferPlayInstance::setLeftVolume(float volume)
{
    m_fixedLeftVolume = (int)(GEMaxAudioVolumeValue * volume);
}


/*!
  Sets \a volume for the right channel. The given argument value should be
  between 0.0 and 1.0 since 1.0 indicates 100 %.
*/
void AudioBufferPlayInstance::setRightVolume(float volume)
{
    m_fixedRightVolume = (int)(GEMaxAudioVolumeValue * volume);
}


/*!
  TODO: Document this method.

  Note: Does not do any bound checking, must be checked before called!
*/
int AudioBufferPlayInstance::mixBlock(AUDIO_SAMPLE_TYPE *target,
                                      int samplesToMix)
{
    SAMPLE_FUNCTION_TYPE sampleFunction = m_buffer->getSampleFunction();

    if (!sampleFunction) {
        // Unsupported sample type.
        return 0;
    }

    AUDIO_SAMPLE_TYPE *t_target = target + samplesToMix * 2;
    int sourcepos(0);

    if (m_buffer->getNofChannels() == 2) {
        // Stereo
        while (target != t_target) {
            sourcepos = m_fixedPos >> 12;

            target[0] = (((((sampleFunction)
                            (m_buffer, sourcepos, 0) *
                            (4096 - (m_fixedPos & 4095)) +
                            (sampleFunction)(m_buffer, sourcepos + 1, 0) *
                            (m_fixedPos & 4095)) >> 12) *
                          m_fixedLeftVolume) >> 12);

            target[1] = (((((sampleFunction)
                            (m_buffer, sourcepos, 1) *
                            (4096 - (m_fixedPos & 4095)) +
                            (sampleFunction)(m_buffer, sourcepos + 1, 1) *
                            (m_fixedPos & 4095) ) >> 12) *
                          m_fixedRightVolume) >> 12);

            m_fixedPos += m_fixedInc;
            target += 2;
        }
    }
    else {
        // Mono
        int temp(0);

        while (target != t_target) {
            sourcepos = m_fixedPos >> 12;

            temp = (((sampleFunction)(m_buffer, sourcepos, 0 ) *
                     (4096 - (m_fixedPos & 4095)) +
                     (sampleFunction)(m_buffer, sourcepos + 1, 0) *
                     (m_fixedPos & 4095)) >> 12);

            target[0] = ((temp * m_fixedLeftVolume) >> 12);
            target[1] = ((temp * m_fixedRightVolume) >> 12);

            m_fixedPos += m_fixedInc;
            target += 2;
        }
    }

    return samplesToMix;
}
