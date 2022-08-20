// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>

#include <QObject>
#include <QWidget>

#include "KaraokeData/Song.h"

class QPaintEvent;

class PlaybackBarWidget final : public QWidget
{
    Q_OBJECT

    using Microseconds = std::chrono::microseconds;

public:
    explicit PlaybackBarWidget(QWidget* parent = nullptr);

    Microseconds GetCurrentTime() const;
    bool IsBeingDragged() const;
    void Update(Microseconds current_time, Microseconds song_length);

signals:
    void Dragged(Microseconds new_time);
    void DragEnd();

public slots:
    void ReloadSong(KaraokeData::Song* song);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private slots:
    void OnLinesChanged(int, int, int, int, int, int);

private:
    Microseconds m_current_time;
    Microseconds m_song_length;

    bool m_being_dragged;
    bool m_drag_relative;
    int m_drag_relative_to_x;
    Microseconds m_drag_relative_to_time;

    KaraokeData::Song* m_song_ref;

    QRect ComputeLineRect() const;
    QRectF ComputeHandleRect() const;
};
