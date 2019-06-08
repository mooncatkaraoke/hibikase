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

    UpdatePlayButtonText();
}

void PlaybackWidget::LoadAudio(std::unique_ptr<QIODevice> io_device)
{
    QByteArray audio_bytes = io_device->readAll();
    io_device->close();

    m_audio_file = std::make_unique<AudioFile>(audio_bytes);
    m_audio_output = std::make_unique<QAudioOutput>(m_audio_file->GetPCMFormat());

    connect(m_audio_output.get(), &QAudioOutput::notify, this, &PlaybackWidget::UpdateTime);
    m_audio_output->setNotifyInterval(10);

    UpdatePlayButtonText();
    UpdateTime();
}

void PlaybackWidget::OnPlayButtonClicked()
{
    if (m_audio_output->state() != QAudio::State::ActiveState)
    {
        m_audio_file->GetPCMBuffer()->reset();
        m_audio_output->start(m_audio_file->GetPCMBuffer());
    }
    else
    {
        m_audio_output->stop();
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
        ms = m_audio_output->processedUSecs() / 1000;
        text = QStringLiteral("%1:%2:%3").arg(ms / 60000,     2, 10, QChar('0'))
                                         .arg(ms / 1000 % 60, 2, 10, QChar('0'))
                                         .arg(ms / 10 % 100,  2, 10, QChar('0'));
    }
    m_time_label->setText(text);

    emit TimeUpdated(std::chrono::milliseconds(ms));
}

void PlaybackWidget::UpdatePlayButtonText()
{
    if (!m_audio_output || m_audio_output->state() != QAudio::State::ActiveState)
        m_play_button->setText(QStringLiteral("Play"));
    else
        m_play_button->setText(QStringLiteral("Stop"));
}
