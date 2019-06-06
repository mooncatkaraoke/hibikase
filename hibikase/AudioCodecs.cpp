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

#include "AudioCodecs.h"

#include <QFile>
#include <QDebug>
#include <QDir>

#define DR_MP3_IMPLEMENTATION
#include "../external/dr_libs/dr_mp3.h"
#define DR_WAV_IMPLEMENTATION
#include "../external/dr_libs/dr_wav.h"

namespace AudioCodecs
{
    AudioFile::AudioFile(QString &filename)
    {
        QFile file(filename);

        drwav_data_format format;
        int16_t *frames = nullptr;
        size_t frames_count;

        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Couldn't open file: '" << filename << "'";
            throw;
        }

        QByteArray bytes = file.readAll();
        file.close();

        drmp3 mp3;
        if (drmp3_init_memory(&mp3, bytes.data(), bytes.length(), nullptr)) {
            drmp3_uninit(&mp3);
            qInfo() << "File is MP3";

            qInfo() << "Decoding MP3 to PCM samples...";

            drmp3_config mp3_config;
            drmp3_int16 *mp3_frames = nullptr;
            drmp3_uint64 mp3_frames_count;
            mp3_frames = drmp3_open_memory_and_read_s16(bytes.data(), bytes.length(), &mp3_config, &mp3_frames_count);
            if (!mp3_frames)
            {
                qWarning() << "Couldn't decode MP3 file";
                throw;
            }

            qInfo() << "Decoded MP3";

            format.channels = mp3_config.outputChannels;
            format.sampleRate = mp3_config.outputSampleRate;
            frames = mp3_frames;
            frames_count = mp3_frames_count;
        }
        else
        {
            qDebug() << "Couldn't load file as MP3";

            qWarning() << "File is not a supported format (MP3)";
            throw;
        }

        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_PCM;
        format.bitsPerSample = 16;

        void *wav_data;
        size_t wav_data_size;
        drwav wav;
        if (!drwav_init_memory_write(&wav, &wav_data, &wav_data_size, &format))
        {
            qWarning() << "Couldn't initialise WAVE writer";
            throw;
        }

        qInfo() << "Generating RIFF WAVE PCM data,"
            << format.bitsPerSample << "bit"
            << format.channels << "channel"
            << format.sampleRate << "Hz,"
            << frames_count << "PCM frames...";

        if (!drwav_write_pcm_frames(&wav, frames_count, frames))
        {
            qWarning() << "Couldn't write WAVE data";
            throw;
        }
        drwav_uninit(&wav);

        qInfo() << "Generated WAVE data";

        m_tempfile.setFileTemplate(QDir::tempPath() + "/XXXXXX.wav");
        m_tempfile.open();
        m_tempfile.write((const char *)wav_data, wav_data_size);
        m_tempfile.close();
        drwav_free(wav_data);

        qInfo() << "Saved WAVE data to temporary file " << m_tempfile.fileName();
    }
}
