/**
 * Copyright (c) 2011 Nokia Corporation.
 *
 * Part of the Qt GameEnabler.
 */

#include "GEAudioBuffer.h"

#include <math.h>
#include <QFile>

#include "GEAudioBufferPlayInstance.h"
#include "GEAudioMixer.h"
#include "trace.h"

using namespace GE;


/*!
  Header for wav data
*/
struct SWavHeader {
    char chunkID[4];
    unsigned int chunkSize;
    char format[4];

    unsigned char subchunk1id[4];
    unsigned int subchunk1size;
    unsigned short audioFormat;
    unsigned short nofChannels;
    unsigned int sampleRate;
    unsigned int byteRate;

    unsigned short blockAlign;
    unsigned short bitsPerSample;

    unsigned char subchunk2id[4];
    unsigned int subchunk2size;
};


/*!
 * \class AudioBuffer
 * \brief A class to hold audio information (a buffer).
 */


/*!
  Constructor.
*/
AudioBuffer::AudioBuffer(QObject *parent /* = 0 */)
    : QObject(parent),
      m_sampleFunction(0),
      m_data(0),
      m_dataLength(0),
      m_nofChannels(0),
      m_bitsPerSample(0),
      m_signedData(false),
      m_samplesPerSec(0)
{
}


/*!
  Destructor.
*/
AudioBuffer::~AudioBuffer()
{
    // Deallocate the data.
    reallocate(0);
}


/*!
  (Re)allocates the audio buffer according to \a length.
*/
void AudioBuffer::reallocate(int length)
{
    if (m_data) {
        delete [] ((char*)m_data);
    }

    m_dataLength = length;

    if (m_dataLength > 0) {
        m_data = new char[m_dataLength];
    }
    else {
        m_data = 0;
    }
}


/*!
  Loads a .wav file from file with \a fileName. Note that this method can be
  used for loading .wav from Qt resources as well. If \a parent is given, it
  is set as the parent of the constructed buffer.

  Returns a new buffer if successful, NULL otherwise.
*/
AudioBuffer *AudioBuffer::loadWav(QString fileName, QObject *parent /* = 0 */)
{
    QFile wavFile(fileName);

    if (wavFile.open(QIODevice::ReadOnly)) {
        AudioBuffer *buffer = loadWav(wavFile, parent);

        if (!buffer) {
            DEBUG_INFO("Failed to load data from " << fileName << "!");
        }

        wavFile.close();
        return buffer;
    }

    DEBUG_INFO("Failed to open " << fileName << ": " << wavFile.errorString());
    return 0;
}


/*!
  Protected method, called from AudioBuffer::loadWav(QString, QObject*).

  Loads a .wav file from a preopened \a wavFile. If \a parent is given, it is
  set as the parent of the constructed buffer.

  Returns a new buffer if successful, NULL otherwise.
*/
AudioBuffer *AudioBuffer::loadWav(QFile &wavFile, QObject *parent /* = 0 */)
{
    if (!wavFile.isOpen()) {
        // The file is not open!
        DEBUG_INFO("The given file must be opened before calling this method!");
        return 0;
    }

    SWavHeader header;

    wavFile.read(header.chunkID, 4);

    if (header.chunkID[0] != 'R' || header.chunkID[1] != 'I' ||
        header.chunkID[2] != 'F' || header.chunkID[3] != 'F') {
        // Incorrect header
        return 0;
    }

    wavFile.read((char*)&header.chunkSize, 4);
    wavFile.read((char*)&header.format, 4);

    if (header.format[0] != 'W' || header.format[1] != 'A' ||
        header.format[2] != 'V' || header.format[3] != 'E') {
        // Incorrect header
        return 0;
    }

    wavFile.read((char*)&header.subchunk1id, 4);

    if (header.subchunk1id[0] != 'f' || header.subchunk1id[1] != 'm' ||
        header.subchunk1id[2] != 't' || header.subchunk1id[3] != ' ') {
        // Incorrect header
        return 0;
    }

    wavFile.read((char*)&header.subchunk1size, 4);
    wavFile.read((char*)&header.audioFormat, 2);
    wavFile.read((char*)&header.nofChannels, 2);
    wavFile.read((char*)&header.sampleRate, 4);
    wavFile.read((char*)&header.byteRate, 4);
    wavFile.read((char*)&header.blockAlign, 2);
    wavFile.read((char*)&header.bitsPerSample, 2);

    while (1) {
        if (wavFile.read((char*)&header.subchunk2id, 4) != 4)
            return 0;

        if (wavFile.read((char*)&header.subchunk2size, 4) != 4)
            return 0;

        if (header.subchunk2id[0] == 'd' && header.subchunk2id[1] == 'a' &&
            header.subchunk2id[2] == 't' && header.subchunk2id[3] == 'a') {
            // Found the data chunk.
            break;
        }

        // This was not the data-chunk. Skip it.
        if (header.subchunk2size < 1) {
            // Error in file!
            return 0;
        }

        char *unused = new char[header.subchunk2size];
        wavFile.read(unused, header.subchunk2size);
        delete [] unused;
    }

    // The data follows.
    if (header.subchunk2size < 1)
        return 0;

    // Construct the buffer.
    AudioBuffer *buffer = new AudioBuffer(parent);

    buffer->m_nofChannels = header.nofChannels;
    buffer->m_bitsPerSample = header.bitsPerSample;
    buffer->m_samplesPerSec = header.sampleRate;
    buffer->m_signedData = 0; // Where to look for this?
    buffer->reallocate(header.subchunk2size);

    wavFile.read((char*)buffer->m_data, header.subchunk2size);

    // Select a good sampling function.
    if (!setSampleFunction(*buffer)) {
        // Failed to resolve the sample function!
        delete buffer;
        return 0;
    }

    return buffer;
}


/*!
  Loads a .wav file from a preopened file handle, \a wavFile. If \a parent is
  given, it is set as the parent of the constructed buffer.

  Returns a new buffer if successful, NULL otherwise.
*/
AudioBuffer *AudioBuffer::loadWav(FILE *wavFile, QObject *parent /* = 0 */)
{
    if (!wavFile) {
        // Invalid file handle!
        return 0;
    }

    // Read the header.
    SWavHeader header;

    fread(header.chunkID, 4, 1, wavFile);

    if (header.chunkID[0] != 'R' || header.chunkID[1] != 'I' ||
        header.chunkID[2] != 'F' || header.chunkID[3] != 'F') {
        // Incorrect header
        return 0;
    }

    fread(&header.chunkSize, 4, 1, wavFile);
    fread(header.format, 4, 1, wavFile);

    if (header.format[0] != 'W' || header.format[1] != 'A' ||
        header.format[2] != 'V' || header.format[3] != 'E') {
        // Incorrect header
        return 0;
    }

    fread(header.subchunk1id, 4, 1, wavFile);

    if (header.subchunk1id[0] != 'f' || header.subchunk1id[1] != 'm' ||
        header.subchunk1id[2] != 't' || header.subchunk1id[3] != ' ') {
        // Incorrect header
        return 0;
    }

    fread(&header.subchunk1size, 4, 1, wavFile);
    fread(&header.audioFormat, 2, 1, wavFile);
    fread(&header.nofChannels, 2, 1, wavFile);
    fread(&header.sampleRate, 4, 1, wavFile);
    fread(&header.byteRate, 4, 1, wavFile);
    fread(&header.blockAlign, 2, 1, wavFile);
    fread(&header.bitsPerSample, 2, 1, wavFile);
    fread(header.subchunk2id, 4, 1, wavFile);

    if (header.subchunk2id[0] != 'd' || header.subchunk2id[1] != 'a' ||
        header.subchunk2id[2] != 't' || header.subchunk2id[3] != 'a') {
        // Incorrect header
        return 0;
    }

    fread(&header.subchunk2size, 4, 1, wavFile);

    // The data follows.
    if (header.subchunk2size < 1)
        return 0;

    AudioBuffer *buffer = new AudioBuffer(parent);

    buffer->m_nofChannels = header.nofChannels;
    buffer->m_bitsPerSample = header.bitsPerSample;
    buffer->m_samplesPerSec = header.sampleRate;
    buffer->m_signedData = 0; // Where to look for this?
    buffer->reallocate(header.subchunk2size);

    fread(buffer->m_data, 1, header.subchunk2size, wavFile);

    // Select a good sampling function.
    if (!setSampleFunction(*buffer)) {
        // Failed to select the sampling function!
        delete buffer;
        return 0;
    }

    return buffer;
}


// Mix to  mono versions.

AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction8bitMono(AudioBuffer *buffer,
                                                      int pos,
                                                      int channel)
{
    Q_UNUSED(channel);
    return (AUDIO_SAMPLE_TYPE)(((quint8*)(buffer->m_data))[pos] - 128) << 8;
}


AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction16bitMono(AudioBuffer *buffer,
                                                        int pos,
                                                        int channel)
{
    Q_UNUSED(channel);
    return (AUDIO_SAMPLE_TYPE)(((quint16*)(buffer->m_data))[pos]);
}


AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction32bitMono(AudioBuffer *buffer,
                                                        int pos,
                                                        int channel)
{
    Q_UNUSED(channel); // To prevent compiler warnings.
    return (((float*)(buffer->m_data))[pos * buffer->m_nofChannels]) * 65536.0f / 2.0f;
}


// Mix to stereo versions.

AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction8bitStereo(AudioBuffer *buffer,
                                                         int pos,
                                                         int channel)
{
    return ((AUDIO_SAMPLE_TYPE)
        (((quint8*)(buffer->m_data))[pos * buffer->m_nofChannels + channel]) << 8);
}


AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction16bitStereo(AudioBuffer *buffer,
                                                          int pos,
                                                          int channel)
{
    return (AUDIO_SAMPLE_TYPE)
        (((quint16*)(buffer->m_data))[pos * buffer->m_nofChannels + channel]);
}


AUDIO_SAMPLE_TYPE AudioBuffer::sampleFunction32bitStereo(AudioBuffer *buffer,
                                                          int pos,
                                                          int channel)
{
    return (((float*)(buffer->m_data))[pos * buffer->m_nofChannels +
            channel]) * 65536.0f / 2.0f;
}


/*!
  Constructs a new play instance and sets it as an audio source for \a mixer.
  Note that the mixer takes ownership of the constructed instance.
  Returns the constructed instance or NULL in case the mixer refused to add
  the instance.
*/
AudioBufferPlayInstance *AudioBuffer::playWithMixer(AudioMixer &mixer)
{
    AudioBufferPlayInstance *instance = new AudioBufferPlayInstance(this);

    if (!mixer.addAudioSource(instance)) {
        DEBUG_INFO("Failed to add the new audio source to mixer!");
        delete instance;
        return NULL;
    }

    return instance;
}


/*!
  Sets an appropriate sample function for \a buffer depending on the number of
  channels and the bit rate.

  Returns true if successful, false otherwise.
*/
bool AudioBuffer::setSampleFunction(AudioBuffer &buffer)
{
    buffer.m_sampleFunction = 0;

    if (buffer.m_nofChannels == 1) {
        if (buffer.m_bitsPerSample == 8)
            buffer.m_sampleFunction = sampleFunction8bitMono;

        if (buffer.m_bitsPerSample == 16)
            buffer.m_sampleFunction = sampleFunction16bitMono;

        if (buffer.m_bitsPerSample == 32)
            buffer.m_sampleFunction = sampleFunction32bitMono;
    } else {
        if (buffer.m_bitsPerSample == 8)
            buffer.m_sampleFunction = sampleFunction8bitStereo;

        if (buffer.m_bitsPerSample == 16)
            buffer.m_sampleFunction = sampleFunction16bitStereo;

        if (buffer.m_bitsPerSample == 32)
            buffer.m_sampleFunction = sampleFunction32bitStereo;
    }

    if (!buffer.m_sampleFunction) {
        // Unknown bit rate!
        DEBUG_INFO("Unknown bit rate:" << buffer.m_bitsPerSample);
        return false;
    }

    return true;
}
