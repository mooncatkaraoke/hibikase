// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>

#include <QAudioFormat>
#include <QByteArray>
#include <QBuffer>

class AudioFile
{
public:
    QString Load(QByteArray bytes);
    QAudioFormat GetPCMFormat() const
    {
        return m_pcm_format;
    }
    QBuffer *GetPCMBuffer() const
    {
        return m_pcm_buffer.get();
    }
private:
    enum AudioType {
        Unknown = 0,
        MP3,
        FLAC,
    } m_type = AudioType::Unknown;
    QAudioFormat m_pcm_format;
    std::unique_ptr<QBuffer> m_pcm_buffer = nullptr;
};
