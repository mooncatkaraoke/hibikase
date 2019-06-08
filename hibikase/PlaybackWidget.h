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
#include <QIODevice>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "AudioFile.h"

class PlaybackWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaybackWidget(QWidget* parent = nullptr);

    void LoadAudio(std::unique_ptr<QIODevice> io_device);  // Can be nullptr

signals:
    void TimeUpdated(std::chrono::milliseconds ms);

private slots:
    void UpdateTime();
    void FixTime();
    void OnPlayButtonClicked();

private:
    void UpdatePlayButtonText();

    QPushButton* m_play_button;
    QLabel* m_time_label;
    QTimer* m_time_fix_timer;
    qint64 m_last_time = -1;

    std::unique_ptr<AudioFile> m_audio_file = nullptr;
    std::unique_ptr<QAudioOutput> m_audio_output = nullptr;
};
