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
#include <vector>

#include <QObject>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QWidget>

#include "KaraokeData/Song.h"

class QPaintEvent;

enum class TimingState
{
    Uninitialized,
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
                                 Milliseconds start_time, Milliseconds end_time);

    void Update(Milliseconds time, bool line_is_inactivating);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void CalculateGeometry();
    QColor GetNotPlayedColor() const;
    QColor GetPlayingColor() const;
    QColor GetPlayedColor() const;

    const QPlainTextEdit* const m_text_edit;
    const int m_start_index;
    const int m_end_index;
    const Milliseconds m_start_time;
    const Milliseconds m_end_time;
    qreal m_progress = 0;
    TimingState m_state = TimingState::Uninitialized;
};

class LineTimingDecorations final : public QObject
{
    Q_OBJECT

public:
    LineTimingDecorations(KaraokeData::Line* line, int position,
                          QPlainTextEdit* text_edit, QObject* parent = nullptr);

    void Update(std::chrono::milliseconds time);
    int GetPosition() const;

private:
    std::vector<std::unique_ptr<SyllableDecorations>> m_syllables;
    // TODO: Use a const reference instead
    KaraokeData::Line* m_line;
    int m_position;
    TimingState m_state = TimingState::Uninitialized;
};
