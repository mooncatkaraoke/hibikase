// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include <QObject>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QWidget>

#include "KaraokeData/Song.h"

class QPaintEvent;

enum class TimingState
{
    NotPlayed,
    Playing,
    Played
};

class SyllableDecorations final : public QWidget
{
    Q_OBJECT

    using Milliseconds = std::chrono::milliseconds;

public:
    explicit SyllableDecorations(const QPlainTextEdit* text_edit, int start_index, int end_index,
                                 Milliseconds start_time, Milliseconds end_time,
                                 bool show_end_marker, TimingState state);

    void Update(Milliseconds time, bool line_is_active);
    int GetPosition() const;
    void AddToPosition(int diff);
    void Relayout();

protected:
    void paintEvent(QPaintEvent*) override;
    void moveEvent(QMoveEvent*) override;

private:
    const QPlainTextEdit* const m_text_edit;
    int m_start_index;
    int m_end_index;
    const Milliseconds m_start_time;
    const Milliseconds m_end_time;
    const bool m_show_end_marker;
    qreal m_progress = 0;
    bool m_line_is_active = false;
    TimingState m_state;
};

class LineTimingDecorations final : public QObject
{
    Q_OBJECT

    using Milliseconds = std::chrono::milliseconds;

public:
    LineTimingDecorations(const KaraokeData::Line& line, int position, QPlainTextEdit* text_edit,
                          Milliseconds time, QObject* parent = nullptr);

    void Update(Milliseconds time);
    int GetPosition() const;
    void AddToPosition(int diff);
    int TextPositionToSyllable(int position) const;
    int TextPositionFromSyllable(int position) const;
    void Relayout();

private:
    std::vector<std::unique_ptr<SyllableDecorations>> m_syllables;
    const KaraokeData::Line& m_line;
    int m_start_index;
    int m_end_index;
    TimingState m_state;
};
