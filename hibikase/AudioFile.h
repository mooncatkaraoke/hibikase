// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <chrono>

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
    std::chrono::microseconds GetDuration() const
    {
        if (!m_pcm_buffer)
            return std::chrono::milliseconds::zero();
        return std::chrono::microseconds(m_pcm_format.durationForBytes(m_pcm_buffer->size()));
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
