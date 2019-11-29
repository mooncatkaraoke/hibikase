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

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontDatabase>

#include <RubberBandStretcher.h>

PlaybackWidget::PlaybackWidget(QWidget* parent) : QWidget(parent)
{
    m_play_button = new QPushButton();
    m_time_label = new QLabel();
    m_time_slider = new QSlider(Qt::Orientation::Horizontal);

    m_time_label->setTextFormat(Qt::PlainText);
    m_time_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    QVBoxLayout* main_layout = new QVBoxLayout();
    QHBoxLayout* sub_layout = new QHBoxLayout();
    main_layout->setMargin(0);

    sub_layout->addWidget(m_time_label);
    sub_layout->addWidget(m_time_slider);
    main_layout->addLayout(sub_layout);
    main_layout->addWidget(m_play_button);

    setLayout(main_layout);

    connect(m_play_button, &QPushButton::clicked, this, &PlaybackWidget::OnPlayButtonClicked);
    connect(m_time_slider, &QSlider::sliderMoved, this, &PlaybackWidget::OnTimeSliderMoved);
    connect(m_time_slider, &QSlider::sliderReleased, this, &PlaybackWidget::OnTimeSliderReleased);
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
    m_time_slider->setEnabled(false);
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
    m_time_slider->setEnabled(true);
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

void PlaybackWidget::OnTimeSliderMoved(int value)
{
    if (!m_time_slider->isSliderDown())
        return;

    // Update karaoke text but not audio when dragging slider
    // (It would also be possible to update audio, but you may not like the sound)
    emit TimeUpdated(std::chrono::milliseconds(value));
}

void PlaybackWidget::OnTimeSliderReleased()
{
    if (!m_worker)
        return;

    std::chrono::microseconds new_pos(m_time_slider->value() * 1000);
    QMetaObject::invokeMethod(m_worker, "Play", Q_ARG(std::chrono::microseconds, new_pos));
}

void PlaybackWidget::OnStateChanged(QAudio::State state)
{
    m_state = state;

    if (state != QAudio::State::ActiveState)
        m_play_button->setText(QStringLiteral("Play"));
    else
        m_play_button->setText(QStringLiteral("Stop"));

    if (state == QAudio::State::StoppedState)
        UpdateTime(std::chrono::microseconds(-1), std::chrono::microseconds(-1));
}

static QString formatTime(std::chrono::microseconds time)
{
    long long val = time.count() / 1000;
    return QStringLiteral("%1:%2:%3").arg(val / 60000,     2, 10, QChar('0'))
                                     .arg(val / 1000 % 60, 2, 10, QChar('0'))
                                     .arg(val / 10 % 100,  2, 10, QChar('0'));
}

void PlaybackWidget::UpdateTime(std::chrono::microseconds current, std::chrono::microseconds length)
{
    QString text;
    // When we request to delete the worker, it doesn't happen immediately;
    // if it triggers time updates in that period we want to ignore them.
    if (m_worker && current.count() >= 0)
    {
        text = QStringLiteral("%1 / %2").arg(formatTime(current), formatTime(length));
        // Don't undo the user's actions when they are dragging the slider
        if (!m_time_slider->isSliderDown())
        {
            // Range and value in milliseconds because `int` is usually 32-bit
            // and INT_MAX is not even enough for 36 minutes in microseconds
            m_time_slider->setRange(0, length.count() / 1000);
            m_time_slider->setValue(current.count() / 1000);
        }
    }
    else
    {
        m_time_slider->setRange(0, 0);
        m_time_slider->setValue(0);
    }
    m_time_label->setText(text);

    // Slider issues time signals itself when being dragged (ignore audio's time signals)
    if (!m_time_slider->isSliderDown())
    {
        emit TimeUpdated(std::chrono::duration_cast<std::chrono::milliseconds>(current));
    }
}

AudioOutputWorker::AudioOutputWorker(std::unique_ptr<QIODevice> io_device, QWidget* parent)
    : QObject(parent), m_io_device(std::move(io_device))
{
    qRegisterMetaType<std::chrono::microseconds>();
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

    m_stretcher_float_buffers.resize(m_audio_output->format().channelCount());
    m_stretcher_float_buffers_pointers.resize(m_audio_output->format().channelCount());

    m_stretcher = std::make_unique<RubberBand::RubberBandStretcher>(
                m_audio_output->format().sampleRate(), m_audio_output->format().channelCount(),
                RubberBand::RubberBandStretcher::Option::OptionProcessRealTime);

    connect(m_audio_output.get(), &QAudioOutput::stateChanged, this, &AudioOutputWorker::OnStateChanged);
    connect(m_audio_output.get(), &QAudioOutput::notify, this, &AudioOutputWorker::OnNotify);

    emit LoadFinished(QString());
}

void AudioOutputWorker::Play(std::chrono::microseconds from)
{
    if (m_audio_output->state() != QAudio::State::ActiveState)
    {
        m_start_offset = 0;
        m_current_offset = 0;
    }

    const qint32 from_bytes = m_audio_output->format().bytesForDuration(from.count());
    m_start_offset += from_bytes - m_current_offset;
    m_current_offset = from_bytes;
    m_last_seek_offset = m_current_offset;

    if (m_audio_output->state() != QAudio::State::ActiveState)
    {
        Stop();
        m_output_buffer = m_audio_output->start();
        PushSamplesToOutput();
    }
}

void AudioOutputWorker::Stop()
{
    m_audio_output->stop();
    m_audio_output->reset();
    m_stretcher->reset();
}

template <typename T>
static void ToFloat(const T* in, std::vector<std::vector<float>>* out)
{
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>, "Incompatible type");
    const float scale = 1.0f / (std::numeric_limits<T>::max() + 1.0f);

    for (size_t i = 0; i < out->size(); ++i)
    {
        const T* ptr = in + i;
        for (size_t j = 0; j < (*out)[i].size(); ++j)
        {
            (*out)[i][j] = *ptr * scale;
            ptr += out->size();
        }
    }
}

template <typename T>
static void FromFloat(const std::vector<std::vector<float>>& in, T* out)
{
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) < sizeof(int),
                  "Incompatible type");
    const int min = std::numeric_limits<T>::min();
    const int max = std::numeric_limits<T>::max();

    for (size_t i = 0; i < in.size(); ++i)
    {
        T* ptr = out + i;
        for (size_t j = 0; j < in[i].size(); ++j)
        {
            *ptr = static_cast<T>(qBound(min, static_cast<int>(in[i][j] * (max + 1)), max));
            ptr += in.size();
        }
    }
}

bool AudioOutputWorker::PushSamplesToStretcher()
{
    const QByteArray& audio_buffer = m_audio_file->GetPCMBuffer()->data();
    if (m_current_offset >= audio_buffer.size())
        return false;

    const size_t bytes_per_frame = m_audio_output->format().bytesPerFrame();
    const size_t frames = std::min<size_t>((audio_buffer.size() - m_current_offset) / bytes_per_frame,
                                           m_stretcher->getSamplesRequired());
    for (size_t i = 0; i < m_stretcher_float_buffers.size(); ++i)
    {
        m_stretcher_float_buffers[i].resize(frames);
        m_stretcher_float_buffers_pointers[i] = m_stretcher_float_buffers[i].data();
    }

    const size_t bytes_per_sample = bytes_per_frame / m_stretcher_float_buffers.size();
    Q_ASSERT(bytes_per_sample == 2 &&
             m_audio_output->format().sampleType() == QAudioFormat::SampleType::SignedInt);
    ToFloat(reinterpret_cast<const int16_t*>(audio_buffer.data() + m_current_offset),
            &m_stretcher_float_buffers);

    m_current_offset += frames * bytes_per_frame;
    const bool final = m_current_offset == audio_buffer.size();
    m_stretcher->process(m_stretcher_float_buffers_pointers.data(), frames, final);

    return true;
}

void AudioOutputWorker::PushSamplesToOutput()
{
    do
    {
        const size_t bytes_per_frame = m_audio_output->format().bytesPerFrame();
        size_t frames = std::min<size_t>(m_audio_output->bytesFree() / bytes_per_frame,
                                         m_stretcher->available());
        if (frames == 0)
            continue;

        const size_t channels = m_stretcher_float_buffers.size();
        for (size_t i = 0; i < m_stretcher_float_buffers.size(); ++i)
        {
            m_stretcher_float_buffers[i].resize(frames);
            m_stretcher_float_buffers_pointers[i] = m_stretcher_float_buffers[i].data();
        }

        frames = m_stretcher->retrieve(m_stretcher_float_buffers_pointers.data(), frames);
        if (frames == 0)
            continue;

        const size_t bytes_per_sample = bytes_per_frame / channels;
        Q_ASSERT(bytes_per_sample == 2 &&
                 m_audio_output->format().sampleType() == QAudioFormat::SampleType::SignedInt);
        m_stretcher_buffer.resize(frames * bytes_per_frame);
        FromFloat(m_stretcher_float_buffers, reinterpret_cast<int16_t*>(m_stretcher_buffer.data()));

        m_output_buffer->write(m_stretcher_buffer.data(), m_stretcher_buffer.size());
    } while (m_audio_output->bytesFree() && PushSamplesToStretcher());
}

std::chrono::microseconds AudioOutputWorker::DurationForBytes(qint32 bytes)
{
    return std::chrono::microseconds((bytes < 0 ? -1 : 1) *
                                     m_audio_output->format().durationForBytes(std::abs(bytes)));
}

void AudioOutputWorker::OnNotify()
{
    const qint64 latency = m_audio_output->format().durationForFrames(
            static_cast<qint32>(m_stretcher->getLatency()));
    const std::chrono::microseconds time = DurationForBytes(m_start_offset) +
            std::chrono::microseconds(m_audio_output->processedUSecs() - latency);
    const std::chrono::microseconds last_seek = DurationForBytes(m_last_seek_offset);

    emit TimeUpdated(std::max(time, last_seek), m_audio_file->GetDuration());

    PushSamplesToOutput();
}

void AudioOutputWorker::OnStateChanged(QAudio::State state)
{
    emit StateChanged(state);
}
