// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// As an additional permission for this file only, you can (at your
// option) instead use this file under the terms of CC0.
// <http://creativecommons.org/publicdomain/zero/1.0/>

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "AudioFile.h"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QIODevice>

#define DR_MP3_IMPLEMENTATION
#include "../external/dr_libs/dr_mp3.h"
#define DR_FLAC_IMPLEMENTATION
#include "../external/dr_libs/dr_flac.h"

AudioFile::AudioFile(QByteArray bytes)
{
    int16_t *frames = nullptr;
    size_t frames_count;

    enum {
        TYPE_UNKNOWN = 0,
        TYPE_MP3,
        TYPE_FLAC,
    } type = TYPE_UNKNOWN;

    drmp3 mp3;
    if (drmp3_init_memory(&mp3, bytes.data(), bytes.length(), nullptr))
    {
        drmp3_uninit(&mp3);

        qInfo() << "File is MP3";

        type = TYPE_MP3;

        qInfo() << "Decoding MP3 to PCM...";

        drmp3_config mp3_config = {0, 0};
        drmp3_uint64 mp3_frames_count;
        drmp3_int16 *mp3_frames = drmp3_open_memory_and_read_s16(bytes.data(), bytes.length(), &mp3_config, &mp3_frames_count);
        if (!mp3_frames)
        {
            qWarning() << "Couldn't decode MP3 file";
            throw;
        }

        qInfo() << "Decoded MP3";

        m_pcm_format.setChannelCount(mp3_config.outputChannels);
        m_pcm_format.setSampleRate(mp3_config.outputSampleRate);
        frames = mp3_frames;
        frames_count = mp3_frames_count;
    }
    else if (drflac *flac = drflac_open_memory(bytes.data(), bytes.length()))
    {
        drflac_close(flac);

        qInfo() << "File is FLAC";

        type = TYPE_FLAC;

        qInfo() << "Decoding FLAC to PCM...";

        unsigned int flac_channels, flac_sample_rate;
        drflac_uint64 flac_frames_count;
        drflac_int16 *flac_frames = drflac_open_memory_and_read_pcm_frames_s16(bytes.data(), bytes.length(), &flac_channels, &flac_sample_rate, &flac_frames_count);
        if (!flac_frames)
        {
            qWarning() << "Couldn't decode FLAC file";
            throw;
        }

        qInfo() << "Decoded FLAC";

        m_pcm_format.setChannelCount(flac_channels);
        m_pcm_format.setSampleRate(flac_sample_rate);
        frames = flac_frames;
        frames_count = flac_frames_count;
    }
    else
    {
        qWarning() << "File is not in a supported format (MP3 or FLAC)";
        throw;
    }

    m_pcm_format.setCodec("audio/pcm");
    m_pcm_format.setSampleSize(16);
    m_pcm_format.setSampleType(QAudioFormat::SampleType::SignedInt);
    // QAudioFormat endianness is default-initialised to the platform one, so
    // we don't need to set it or convert the endianness ourselves!

    bytes = QByteArray((const char*)frames, frames_count * m_pcm_format.bytesPerFrame());

    if (type == TYPE_MP3)
        drmp3_free(frames);
    else if (type == TYPE_FLAC)
        drflac_free(frames);
    else
        assert(0 && "Unhandled case?!");

    m_pcm_buffer = std::make_unique<QBuffer>();
    m_pcm_buffer->setData(bytes);
    m_pcm_buffer->open(QIODevice::OpenModeFlag::ReadOnly);

    qInfo() << "PCM data is"
        << m_pcm_format.sampleSize() << "bit"
        << m_pcm_format.channelCount() << "channel"
        << m_pcm_format.sampleRate() << "Hz,"
        << frames_count << "frames...";
}
