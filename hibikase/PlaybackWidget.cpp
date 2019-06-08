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

#include "PlaybackWidget.h"

#include <utility>

#include <QDebug>
#include <QTimer>
#include <QVBoxLayout>

PlaybackWidget::PlaybackWidget(QWidget* parent) : QWidget(parent)
{
    m_play_button = new QPushButton();
    m_time_label = new QLabel();

    m_time_label->setTextFormat(Qt::PlainText);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setMargin(0);
    main_layout->addWidget(m_play_button);
    main_layout->addWidget(m_time_label);

    setLayout(main_layout);

    connect(m_play_button, &QPushButton::clicked, this, &PlaybackWidget::OnPlayButtonClicked);

    m_time_fix_timer = new QTimer(this);
    connect(m_time_fix_timer, &QTimer::timeout, this, &PlaybackWidget::FixTime);

    UpdatePlayButtonText();
}

void PlaybackWidget::LoadAudio(std::unique_ptr<QIODevice> io_device)
{
    QByteArray audio_bytes = io_device->readAll();
    io_device->close();

    m_audio_file = std::make_unique<AudioFile>(audio_bytes);
    m_audio_output = std::make_unique<QAudioOutput>(m_audio_file->GetPCMFormat());

    connect(m_audio_output.get(), &QAudioOutput::notify, this, &PlaybackWidget::UpdateTime);
    m_audio_output->setNotifyInterval(1000 / 60);

    m_time_fix_timer->stop();

    UpdatePlayButtonText();
    UpdateTime();
}

void PlaybackWidget::OnPlayButtonClicked()
{
    if (m_audio_output->state() != QAudio::State::ActiveState)
    {
        m_audio_file->GetPCMBuffer()->reset();
        m_audio_output->start(m_audio_file->GetPCMBuffer());

        m_time_fix_timer->start(1000);
    }
    else
    {
        m_audio_output->stop();
        m_time_fix_timer->stop();
    }

    UpdatePlayButtonText();
    UpdateTime();
}

void PlaybackWidget::UpdateTime()
{
    QString text;
    qint64 ms = -1;
    if (m_audio_output->state() != QAudio::State::StoppedState)
    {
        m_last_time = m_audio_output->processedUSecs();
        ms = m_last_time / 1000;
        text = QStringLiteral("%1:%2:%3").arg(ms / 60000,     2, 10, QChar('0'))
                                         .arg(ms / 1000 % 60, 2, 10, QChar('0'))
                                         .arg(ms / 10 % 100,  2, 10, QChar('0'));
    }
    m_time_label->setText(text);

    emit TimeUpdated(std::chrono::milliseconds(ms));
}

void PlaybackWidget::FixTime()
{
    // This signal callback works around an issue observed on macOS where, in
    // some cases where the UI thread was blocked for some short period of
    // time, the playback time will stop updating.
    // So we check if it seems to be lagging and try to jolt it into working
    // again.
    if (m_last_time != -1 && m_audio_output
        && m_audio_output->state() == QAudio::State::ActiveState
        && m_audio_output->processedUSecs() - m_last_time > 500000) // 500ms
    {
        qInfo() << "Audio playback position updates seem to have stopped,"
                << "suspending and resuming audio output to try to fix.";
        m_audio_output->suspend();
        m_audio_output->resume();
    }
}

void PlaybackWidget::UpdatePlayButtonText()
{
    if (!m_audio_output || m_audio_output->state() != QAudio::State::ActiveState)
        m_play_button->setText(QStringLiteral("Play"));
    else
        m_play_button->setText(QStringLiteral("Stop"));
}
