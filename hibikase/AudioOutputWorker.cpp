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

#include "AudioOutputWorker.h"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

#include <QByteArray>

AudioOutputWorker::AudioOutputWorker(std::unique_ptr<QIODevice> io_device, QObject* parent)
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

void AudioOutputWorker::Play()
{
    if (m_audio_output->state() != QAudio::State::StoppedState)
    {
        m_audio_output->resume();
    }
    else
    {
        m_start_offset = 0;
        m_current_offset = 0;
        m_last_seek_offset = 0;
        m_last_speed_change_offset = 0;

        m_output_buffer = m_audio_output->start();
        PushSamplesToOutput();
    }
}

void AudioOutputWorker::Seek(std::chrono::microseconds to)
{
    const qint32 offset = m_audio_output->format().bytesForDuration(to.count());
    m_start_offset += static_cast<qint32>((offset - m_current_offset) *
                                          m_stretcher->getTimeRatio());
    m_current_offset = offset;
    m_last_seek_offset = offset;

    // Make sure to emit at least one TimeUpdated after seeking. OnNotify won't do it when suspended
    emit TimeUpdated(DurationForBytes(offset), m_audio_file->GetDuration());
}

void AudioOutputWorker::Pause()
{
    m_audio_output->suspend();
}

void AudioOutputWorker::Stop()
{
    m_audio_output->stop();
    m_audio_output->reset();
    m_stretcher->reset();
}

void AudioOutputWorker::SetSpeed(double slowdown)
{
    const qint32 duration = m_current_offset - m_last_speed_change_offset;
    m_start_offset -= static_cast<qint32>(duration * (m_stretcher->getTimeRatio() - 1));
    m_last_speed_change_offset = m_current_offset;

    m_stretcher->setTimeRatio(slowdown);
}

template <typename T>
static void ToFloat(const T* in, std::vector<std::vector<float>>* out)
{
    static_assert(std::is_integral<T>() && std::is_signed<T>(), "Incompatible type");
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
    static_assert(std::is_integral<T>() && std::is_signed<T>() && sizeof(T) < sizeof(int),
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

    if (m_current_offset == m_audio_file->GetPCMBuffer()->size())
        Stop();
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
    const std::chrono::microseconds last_speed_change = DurationForBytes(m_last_speed_change_offset);
    const std::chrono::microseconds scaled_time = last_speed_change +
            std::chrono::microseconds(static_cast<long long>(
                    (time - last_speed_change).count() / m_stretcher->getTimeRatio()));
    const std::chrono::microseconds last_seek = DurationForBytes(m_last_seek_offset);

    emit TimeUpdated(std::max(scaled_time, last_seek), m_audio_file->GetDuration());

    PushSamplesToOutput();
}

void AudioOutputWorker::OnStateChanged(QAudio::State state)
{
    emit StateChanged(state);
}
