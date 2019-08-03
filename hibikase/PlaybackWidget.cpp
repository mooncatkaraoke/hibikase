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
    LoadAudio(nullptr);

    m_thread.start();
}

PlaybackWidget::~PlaybackWidget()
{
    if (m_worker)
        connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_thread.quit();
    m_thread.wait();
}

void PlaybackWidget::LoadAudio(std::unique_ptr<QIODevice> io_device)
{
    if (m_worker)
    {
        disconnect(m_worker, &AudioOutputWorker::StateChanged, this, &PlaybackWidget::OnStateChanged);
        disconnect(m_worker, &AudioOutputWorker::TimeUpdated, this, &PlaybackWidget::UpdateTime);
        QMetaObject::invokeMethod(m_worker, "deleteLater");
        m_worker = nullptr;
    }

    m_play_button->setEnabled(false);
    OnStateChanged(QAudio::State::StoppedState);

    if (!io_device)
    {
        m_play_button->setText("(No audio loaded)");
        return;
    }

    m_play_button->setText("(Loading audio...)");

    m_worker = new AudioOutputWorker(std::move(io_device));
    m_worker->moveToThread(&m_thread);

    connect(m_worker, &AudioOutputWorker::LoadFinished, this, &PlaybackWidget::OnLoadResult);
    QMetaObject::invokeMethod(m_worker, "Initialize");

    connect(m_worker, &AudioOutputWorker::StateChanged, this, &PlaybackWidget::OnStateChanged);
    connect(m_worker, &AudioOutputWorker::TimeUpdated, this, &PlaybackWidget::UpdateTime);
}

void PlaybackWidget::OnLoadResult(QString result)
{
    if (!result.isEmpty())
    {
        m_play_button->setText("(Could not load audio: " + result + ")");
        return;
    }

    m_play_button->setEnabled(true);
    OnStateChanged(QAudio::State::StoppedState);
}

void PlaybackWidget::OnPlayButtonClicked()
{
    if (!m_worker)
        return;

    if (m_state != QAudio::State::ActiveState)
        QMetaObject::invokeMethod(m_worker, "Play");
    else
        QMetaObject::invokeMethod(m_worker, "Stop");
}

void PlaybackWidget::OnStateChanged(QAudio::State state)
{
    m_state = state;

    if (state != QAudio::State::ActiveState)
        m_play_button->setText(QStringLiteral("Play"));
    else
        m_play_button->setText(QStringLiteral("Stop"));

    if (state == QAudio::State::StoppedState)
        UpdateTime(std::chrono::milliseconds(-1));
}

void PlaybackWidget::UpdateTime(std::chrono::milliseconds ms)
{
    QString text;
    // When we request to delete the worker, it doesn't happen immediately;
    // if it triggers time updates in that period we want to ignore them.
    if (m_worker && ms.count() >= 0)
    {
        long long time = ms.count();
        text = QStringLiteral("%1:%2:%3").arg(time / 60000,     2, 10, QChar('0'))
                                         .arg(time / 1000 % 60, 2, 10, QChar('0'))
                                         .arg(time / 10 % 100,  2, 10, QChar('0'));
    }
    m_time_label->setText(text);

    emit TimeUpdated(ms);
}

AudioOutputWorker::AudioOutputWorker(std::unique_ptr<QIODevice> io_device, QWidget* parent)
    : QObject(parent), m_io_device(std::move(io_device))
{
    qRegisterMetaType<std::chrono::milliseconds>();
}

void AudioOutputWorker::Initialize()
{
    QByteArray audio_bytes = m_io_device->readAll();
    m_io_device->close();
    m_io_device.reset();

    m_audio_file = std::make_unique<AudioFile>();
    QString result = m_audio_file->Load(audio_bytes);
    if (!result.isEmpty())
    {
        emit AudioOutputWorker::LoadFinished(result);
        return;
    }

    m_audio_output = std::make_unique<QAudioOutput>(m_audio_file->GetPCMFormat(), this);
    m_audio_output->setNotifyInterval(10);

    connect(m_audio_output.get(), &QAudioOutput::stateChanged, this, &AudioOutputWorker::StateChanged);
    connect(m_audio_output.get(), &QAudioOutput::notify, this, &AudioOutputWorker::OnNotify);

    emit LoadFinished(QString());
}

void AudioOutputWorker::Play()
{
    m_audio_file->GetPCMBuffer()->reset();
    m_audio_output->start(m_audio_file->GetPCMBuffer());
}

void AudioOutputWorker::Stop()
{
    m_audio_output->stop();
}

void AudioOutputWorker::OnNotify()
{
    emit TimeUpdated(std::chrono::milliseconds(m_audio_output->processedUSecs() / 1000));
}
