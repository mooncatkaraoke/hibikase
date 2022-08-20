// SPDX-License-Identifier: GPL-2.0-or-later

// This file implements a slider control that represents the current playback
// position. Hibikase previously used QSlider for this, but its behaviour is not
// consistent across platforms, and it can be quite glitchy.

#include <algorithm>
#include <chrono>

#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPoint>
#include <QRect>

#include "PlaybackBarWidget.h"

using Microseconds = std::chrono::microseconds;

const int HEIGHT = 20;
const int LINE_HEIGHT = 4;
const int HANDLE_WIDTH = 8;

PlaybackBarWidget::PlaybackBarWidget(QWidget *parent)
    : QWidget(parent), m_current_time{}, m_song_length{}, m_being_dragged(false)
{
    setMinimumHeight(HEIGHT);
    setVisible(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

Microseconds PlaybackBarWidget::GetCurrentTime() const
{
    return m_current_time;
}

bool PlaybackBarWidget::IsBeingDragged() const
{
    return m_being_dragged;
}

void PlaybackBarWidget::Update(Microseconds current_time, Microseconds song_length)
{
    m_current_time = current_time;
    m_song_length = song_length;
    update();
}

QRect PlaybackBarWidget::ComputeLineRect() const
{
    int w = width() - HANDLE_WIDTH;
    int h = LINE_HEIGHT;

    return QRect { (width() - w) / 2, (HEIGHT - h) / 2, w, h };
}

QRectF PlaybackBarWidget::ComputeHandleRect() const
{
    QRect line_rect = ComputeLineRect();
    qreal time = (double(m_current_time.count()) / m_song_length.count());
    return QRectF { line_rect.width() * time, 0, qreal(HANDLE_WIDTH), qreal(HEIGHT) };
}

void PlaybackBarWidget::paintEvent(QPaintEvent* e)
{
    if (m_song_length.count() == 0)
        return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    auto line_rect = ComputeLineRect();
    auto handle_rect = ComputeHandleRect();
    painter.fillRect(line_rect, Qt::gray);
    painter.fillRect(handle_rect, QPalette().color(QPalette::Text));

    QWidget::paintEvent(e);
}

void PlaybackBarWidget::mousePressEvent(QMouseEvent* e)
{
    m_being_dragged = true;

    QRectF handle_rect = ComputeHandleRect();
    int handle_x_min = int(std::floor(handle_rect.x()));
    int handle_x_max = int(std::ceil(handle_rect.x() + handle_rect.width()));
    int mouse_x = e->x();
    // If the user grabbed the handle, update the time relative to movement of
    // the handle. (If they don't move the handle, don't change it at all.)
    if (handle_x_min <= mouse_x && mouse_x < handle_x_max)
    {
        m_drag_relative = true;
        m_drag_relative_to_x = mouse_x;
        m_drag_relative_to_time = m_current_time;
    }
    // Otherwise: jump to the point on the bar where the user clicked
    else
    {
        m_drag_relative = false;
    }

    mouseMoveEvent(e);
}

template <class Rep, class Period>
static std::chrono::duration<Rep, Period> operator*(
    std::chrono::duration<Rep, Period> a, double b)
{
    return std::chrono::duration<Rep, Period>{Rep(a.count() * b)};
}

void PlaybackBarWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_being_dragged)
        return;

    QRect line_rect = ComputeLineRect();

    if (m_drag_relative)
    {
        int mouse_x = e->x();
        int relative_x = mouse_x - m_drag_relative_to_x;
        double offset = double(relative_x) / line_rect.width();
        m_current_time = std::min(std::max(m_drag_relative_to_time + m_song_length * offset, Microseconds::zero()), m_song_length);
    }
    else
    {
        double new_time = double(e->x() - line_rect.x()) / line_rect.width();
        new_time = std::max(std::min(new_time, 1.0), 0.0);
        m_current_time = m_song_length * new_time;
    }

    update();
    emit PlaybackBarWidget::Dragged(m_current_time);
}

void PlaybackBarWidget::mouseReleaseEvent(QMouseEvent*)
{
    m_being_dragged = false;
    emit PlaybackBarWidget::DragEnd();
}
