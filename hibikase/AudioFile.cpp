// SPDX-License-Identifier: GPL-2.0-or-later

#include "AudioFile.h"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QIODevice>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

QString AudioFile::Load(QByteArray bytes)
{
    int16_t *frames = nullptr;
    size_t frames_count;

    AudioType type = AudioType::Unknown;
    QAudioFormat format;
    drmp3 mp3;
    if (drmp3_init_memory(&mp3, bytes.data(), bytes.length(), nullptr))
    {
        drmp3_uninit(&mp3);

        qInfo() << "File is MP3";

        type = AudioType::MP3;

        qInfo() << "Decoding MP3 to PCM...";

        drmp3_config mp3_config = {0, 0};
        drmp3_uint64 mp3_frames_count;
        drmp3_int16 *mp3_frames = drmp3_open_memory_and_read_s16(bytes.data(), bytes.length(), &mp3_config, &mp3_frames_count);
        if (!mp3_frames)
        {
            qWarning() << "Couldn't decode MP3 file";
            return "Couldn't decode MP3";
        }

        qInfo() << "Decoded MP3";

        format.setChannelCount(mp3_config.outputChannels);
        format.setSampleRate(mp3_config.outputSampleRate);
        frames = mp3_frames;
        frames_count = mp3_frames_count;
    }
    else if (drflac *flac = drflac_open_memory(bytes.data(), bytes.length()))
    {
        drflac_close(flac);

        qInfo() << "File is FLAC";

        type = AudioType::FLAC;

        qInfo() << "Decoding FLAC to PCM...";

        unsigned int flac_channels, flac_sample_rate;
        drflac_uint64 flac_frames_count;
        drflac_int16 *flac_frames = drflac_open_memory_and_read_pcm_frames_s16(bytes.data(), bytes.length(), &flac_channels, &flac_sample_rate, &flac_frames_count);
        if (!flac_frames)
        {
            qWarning() << "Couldn't decode FLAC file";
            return "Couldn't decode FLAC";
        }

        qInfo() << "Decoded FLAC";

        format.setChannelCount(flac_channels);
        format.setSampleRate(flac_sample_rate);
        frames = flac_frames;
        frames_count = flac_frames_count;
    }
    else
    {
        qWarning() << "File is not in a supported format (MP3 or FLAC)";
        return "Unsupported format (not MP3 or FLAC)";
    }

    format.setCodec("audio/pcm");
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SampleType::SignedInt);
    // QAudioFormat endianness is default-initialised to the platform one, so
    // we don't need to set it or convert the endianness ourselves!

    bytes = QByteArray((const char*)frames, frames_count * format.bytesPerFrame());

    if (type == MP3)
        drmp3_free(frames);
    else if (type == FLAC)
        drflac_free(frames);
    else
        assert(0 && "Unhandled case?!");

    m_pcm_buffer = std::make_unique<QBuffer>();
    m_pcm_buffer->setData(bytes);
    m_pcm_buffer->open(QIODevice::OpenModeFlag::ReadOnly);

    m_pcm_format = format;
    m_type = type;

    qInfo() << "PCM data is"
        << m_pcm_format.sampleSize() << "bit"
        << m_pcm_format.channelCount() << "channel"
        << m_pcm_format.sampleRate() << "Hz,"
        << frames_count << "frames";

    return QString();
}
