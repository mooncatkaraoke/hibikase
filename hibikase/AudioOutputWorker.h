// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include <QAudio>
#include <QAudioOutput>
#include <QIODevice>
#include <QObject>

#include <RubberBandStretcher.h>

#include "AudioFile.h"

class AudioOutputWorker final : public QObject
{
    Q_OBJECT

public:
    explicit AudioOutputWorker(std::unique_ptr<QIODevice> io_device, QObject* parent = nullptr);

    enum class PlaybackState
    {
        Stopped,
        Paused,
        Playing,
    };

public slots:
    void Initialize();
    void Play();
    void Seek(std::chrono::microseconds to);
    void Pause();
    void Stop();
    void SetSpeed(double slowdown);

signals:
    void LoadFinished(QString error);
    void PlaybackStateChanged(PlaybackState state);
    void TimeUpdated(std::chrono::microseconds current, std::chrono::microseconds length);

private slots:
    void OnNotify();
    void OnStateChanged(QAudio::State state);

private:
    bool PushSamplesToStretcher();
    void PushSamplesToOutput();
    std::chrono::microseconds DurationForBytes(qint32 bytes);

    std::unique_ptr<QIODevice> m_io_device;
    std::unique_ptr<AudioFile> m_audio_file;
    std::unique_ptr<RubberBand::RubberBandStretcher> m_stretcher;
    std::unique_ptr<QAudioOutput> m_audio_output;
    std::vector<char> m_stretcher_buffer;
    std::vector<std::vector<float>> m_stretcher_float_buffers;
    std::vector<float*> m_stretcher_float_buffers_pointers;
    QIODevice* m_output_buffer = nullptr;
    qint32 m_start_offset;
    qint32 m_current_offset;
    qint32 m_last_seek_offset;
    qint32 m_last_speed_change_offset;
};

Q_DECLARE_METATYPE(std::chrono::microseconds);
Q_DECLARE_METATYPE(AudioOutputWorker::PlaybackState);
