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

#include <chrono>
#include <memory>

#include <QAudioOutput>
#include <QByteArray>
#include <QIODevice>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QWidget>

#include <RubberBandStretcher.h>

#include "AudioFile.h"

class AudioOutputWorker final : public QObject
{
    Q_OBJECT

public:
    explicit AudioOutputWorker(std::unique_ptr<QIODevice> io_device, QWidget* parent = nullptr);

public slots:
    void Initialize();
    void Play(std::chrono::microseconds from = std::chrono::microseconds::zero());
    void Stop();
    void SetSpeed(double slowdown);

signals:
    void LoadFinished(QString error);
    void StateChanged(QAudio::State state);
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
    QIODevice* m_output_buffer;
    qint32 m_start_offset;
    qint32 m_current_offset;
    qint32 m_last_seek_offset;
    qint32 m_last_speed_change_offset;
};

class PlaybackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackWidget(QWidget* parent = nullptr);
    ~PlaybackWidget();

    void LoadAudio(std::unique_ptr<QIODevice> io_device);  // Can be nullptr

signals:
    void TimeUpdated(std::chrono::milliseconds current);

private slots:
    void OnLoadResult(QString result);
    void OnPlayButtonClicked();
    void OnTimeSliderMoved(int value);
    void OnTimeSliderReleased();
    void OnSpeedSliderMoved(int value);
    void OnStateChanged(QAudio::State state);
    void UpdateTime(std::chrono::microseconds current, std::chrono::microseconds length);

private:
    void UpdateSpeedLabel(int value);

    QPushButton* m_play_button;
    QLabel* m_time_label;
    QLabel* m_speed_label;
    QSlider* m_time_slider;
    QSlider* m_speed_slider;

    AudioOutputWorker* m_worker = nullptr;
    QThread m_thread;

    QAudio::State m_state;
};

Q_DECLARE_METATYPE(std::chrono::microseconds);
