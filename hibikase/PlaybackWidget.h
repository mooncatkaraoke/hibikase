// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QThread>
#include <QWidget>

#include "AudioOutputWorker.h"
#include "PlaybackBarWidget.h"

class PlaybackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlaybackWidget(QWidget* parent = nullptr);
    ~PlaybackWidget();

    void LoadAudio(std::unique_ptr<QIODevice> io_device);  // Can be nullptr

signals:
    void TimeUpdated(std::chrono::milliseconds time);
    void SpeedUpdated(double speed);

private slots:
    void OnLoadResult(QString result);
    void OnPlayButtonClicked();
    void OnStopButtonClicked();
    void OnPlaybackBarDragged(std::chrono::microseconds new_time);
    void OnPlaybackBarReleased();
    void OnSpeedSliderUpdated(int value);
    void OnStateChanged(AudioOutputWorker::PlaybackState state);
    void UpdateTime(std::chrono::microseconds current, std::chrono::microseconds length);

private:
    void UpdateSpeedLabel(int value);
    void UpdateSpeed(int value);

    QPushButton* m_play_button;
    QPushButton* m_stop_button;
    QLabel* m_time_label;
    QLabel* m_speed_label;
    PlaybackBarWidget* m_playback_bar;
    QSlider* m_speed_slider;

    AudioOutputWorker* m_worker = nullptr;
    QThread m_thread;

    AudioOutputWorker::PlaybackState m_state;
};
