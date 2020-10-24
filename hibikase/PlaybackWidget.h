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

#include <QAudio>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QWidget>

#include "AudioOutputWorker.h"

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
    void OnStopButtonClicked();
    void OnTimeSliderMoved(int value);
    void OnTimeSliderReleased();
    void OnSpeedSliderMoved(int value);
    void OnStateChanged(QAudio::State state);
    void UpdateTime(std::chrono::microseconds current, std::chrono::microseconds length);

private:
    void UpdateSpeedLabel(int value);

    QPushButton* m_play_button;
    QPushButton* m_stop_button;
    QLabel* m_time_label;
    QLabel* m_speed_label;
    QSlider* m_time_slider;
    QSlider* m_speed_slider;

    AudioOutputWorker* m_worker = nullptr;
    QThread m_thread;

    QAudio::State m_state;
};
