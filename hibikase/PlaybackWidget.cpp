// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlaybackWidget.h"

#include <utility>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontDatabase>

static double ConvertSpeed(int speed)
{
    return 1 / (static_cast<double>(speed) * 0.1);
}

PlaybackWidget::PlaybackWidget(QWidget* parent) : QWidget(parent)
{
    m_play_button = new QPushButton(this);
    m_stop_button = new QPushButton(QStringLiteral("Stop"), this);
    m_time_label = new QLabel(this);
    m_speed_label = new QLabel(this);
    m_playback_bar = new PlaybackBarWidget(this);
    m_speed_slider = new QSlider(Qt::Orientation::Horizontal, this);

    m_play_button->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_stop_button->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    m_speed_slider->setFocusPolicy(Qt::FocusPolicy::TabFocus);

    // We want a font where all numbers have the same width, so that labels don't change in size
    m_time_label->setTextFormat(Qt::PlainText);
    m_time_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_speed_label->setTextFormat(Qt::PlainText);
    m_speed_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    m_speed_slider->setRange(1, 10);
    m_speed_slider->setValue(10);
    UpdateSpeedLabel(m_speed_slider->value());

    QVBoxLayout* main_layout = new QVBoxLayout();
    QHBoxLayout* first_row = new QHBoxLayout();
    QHBoxLayout* second_row = new QHBoxLayout();
    main_layout->setMargin(0);

    first_row->addWidget(m_time_label);
    first_row->addWidget(m_playback_bar);
    second_row->addWidget(m_play_button, 1);
    second_row->addWidget(m_stop_button, 1);
    second_row->addWidget(m_speed_label, 0);
    second_row->addWidget(m_speed_slider, 1);
    main_layout->addLayout(first_row);
    main_layout->addLayout(second_row);

    setLayout(main_layout);

    connect(m_play_button, &QPushButton::clicked, this, &PlaybackWidget::OnPlayButtonClicked);
    connect(m_stop_button, &QPushButton::clicked, this, &PlaybackWidget::OnStopButtonClicked);
    connect(m_playback_bar, &PlaybackBarWidget::Dragged, this, &PlaybackWidget::OnPlaybackBarDragged);
    connect(m_playback_bar, &PlaybackBarWidget::DragEnd, this, &PlaybackWidget::OnPlaybackBarReleased);
    connect(m_speed_slider, &QSlider::valueChanged, this, &PlaybackWidget::OnSpeedSliderUpdated);
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
        disconnect(m_worker, &AudioOutputWorker::PlaybackStateChanged, this, &PlaybackWidget::OnStateChanged);
        disconnect(m_worker, &AudioOutputWorker::TimeUpdated, this, &PlaybackWidget::UpdateTime);
        QMetaObject::invokeMethod(m_worker, "deleteLater");
        m_worker = nullptr;
    }

    m_play_button->setEnabled(false);
    m_stop_button->setEnabled(false);
    OnStateChanged(AudioOutputWorker::PlaybackState::Stopped);

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

    connect(m_worker, &AudioOutputWorker::PlaybackStateChanged, this, &PlaybackWidget::OnStateChanged);
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
    OnStateChanged(AudioOutputWorker::PlaybackState::Stopped);
}

void PlaybackWidget::OnPlayButtonClicked()
{
    if (!m_worker)
        return;

    if (m_state != AudioOutputWorker::PlaybackState::Playing)
    {
        UpdateSpeed(m_speed_slider->value());
        QMetaObject::invokeMethod(m_worker, "Play");
    }
    else
    {
        QMetaObject::invokeMethod(m_worker, "Pause");
    }
}

void PlaybackWidget::OnStopButtonClicked()
{
    if (m_worker)
        QMetaObject::invokeMethod(m_worker, "Stop");
}

void PlaybackWidget::OnPlaybackBarDragged(std::chrono::microseconds value)
{
    // Update karaoke text but not audio when dragging slider
    // (It would also be possible to update audio, but you may not like the sound)
    emit TimeUpdated(std::chrono::duration_cast<std::chrono::milliseconds>(value));
}

void PlaybackWidget::OnPlaybackBarReleased()
{
    if (!m_worker)
        return;

    std::chrono::microseconds new_pos(m_playback_bar->GetCurrentTime());
    QMetaObject::invokeMethod(m_worker, "Seek", Q_ARG(std::chrono::microseconds, new_pos));
}

void PlaybackWidget::OnSpeedSliderUpdated(int value)
{
    UpdateSpeedLabel(value);
    UpdateSpeed(value);
}

void PlaybackWidget::UpdateSpeedLabel(int value)
{
    m_speed_label->setText(QStringLiteral("Speed: %1%").arg(QString::number(value * 10), 3, QChar(' ')));
}

void PlaybackWidget::UpdateSpeed(int value)
{
    const double speed = ConvertSpeed(value);

    emit SpeedUpdated(speed);

    if (m_worker)
        QMetaObject::invokeMethod(m_worker, "SetSpeed", Q_ARG(double, speed));
}

void PlaybackWidget::OnStateChanged(AudioOutputWorker::PlaybackState state)
{
    m_state = state;

    if (state != AudioOutputWorker::PlaybackState::Playing)
        m_play_button->setText(QStringLiteral("Play"));
    else
        m_play_button->setText(QStringLiteral("Pause"));

    m_stop_button->setEnabled(state != AudioOutputWorker::PlaybackState::Stopped);

    if (state == AudioOutputWorker::PlaybackState::Stopped)
        UpdateTime(std::chrono::microseconds(0), std::chrono::microseconds(0));
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
        // Don't fight the user when they are dragging the slider
        if (!m_playback_bar->IsBeingDragged())
        {
            m_playback_bar->Update(current, length);
        }
    }
    else
    {
        m_playback_bar->Update(current, length);
    }
    m_time_label->setText(text);

    // Slider issues time signals itself when being dragged (ignore audio's time signals)
    if (!m_playback_bar->IsBeingDragged())
    {
        emit TimeUpdated(std::chrono::duration_cast<std::chrono::milliseconds>(current));
    }
}
