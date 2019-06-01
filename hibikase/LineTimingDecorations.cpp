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

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>

#include <QBrush>
#include <QColor>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>

#include "KaraokeData/Song.h"

#include "LineTimingDecorations.h"

static constexpr int SYLLABLE_MARKER_WIDTH = 4;
static constexpr int SYLLABLE_MARKER_HEIGHT = 4;
static constexpr int PROGRESS_LINE_HEIGHT = 1;

static const QColor COLOR_NOT_PLAYING(0x77, 0x55, 0x77);
static const QColor COLOR_PLAYING(0x00, 0x00, 0x00);
static const QColor COLOR_PLAYED(0x55, 0x77, 0x55);

using Milliseconds = std::chrono::milliseconds;

static TimingState GetTimingState(Milliseconds current, Milliseconds start, Milliseconds end)
{
    if (start > current)
        return TimingState::NotPlayed;
    else if (end > current)
        return TimingState::Playing;
    else
        return TimingState::Played;
}

SyllableDecorations::SyllableDecorations(const QPlainTextEdit* text_edit, int start_index,
        int end_index, Milliseconds start_time, Milliseconds end_time)
    : QWidget(text_edit->viewport()), m_text_edit(text_edit), m_start_index(start_index),
      m_end_index(end_index), m_start_time(start_time), m_end_time(end_time)
{
    CalculateGeometry();

    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setVisible(true);
}

void SyllableDecorations::Update(Milliseconds time, bool line_is_inactivating)
{
    const TimingState state = GetTimingState(time, m_start_time, m_end_time);
    if (!line_is_inactivating && state == m_state && state != TimingState::Playing)
        return;

    if (!line_is_inactivating && m_start_time <= time)
    {
        m_progress = std::min<qreal>(1.0, static_cast<qreal>((time - m_start_time).count()) /
                                          (m_end_time - m_start_time).count());
    }
    else
    {
        m_progress = 0;
    }

    update();

    if (state == m_state)
        return;
    m_state = state;

    QTextCursor cursor(m_text_edit->document());
    cursor.setPosition(m_start_index, QTextCursor::MoveAnchor);
    cursor.setPosition(m_end_index, QTextCursor::KeepAnchor);

    QTextCharFormat color;
    if (state == TimingState::NotPlayed)
        color.setForeground(COLOR_NOT_PLAYING);
    else if (state == TimingState::Playing)
        color.setForeground(COLOR_PLAYING);
    else
        color.setForeground(COLOR_PLAYED);
    cursor.setCharFormat(color);
}

void SyllableDecorations::paintEvent(QPaintEvent*)
{
    CalculateGeometry();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::gray));

    if (m_progress != 0)
    {
        const int max_width = width() - SYLLABLE_MARKER_WIDTH;
        painter.drawRect(QRectF(SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT,
                                max_width * m_progress, PROGRESS_LINE_HEIGHT));
    }

    QPainterPath path;
    path.moveTo(QPoint(SYLLABLE_MARKER_WIDTH, 0));
    path.lineTo(QPoint(0, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(SYLLABLE_MARKER_WIDTH, 0));

    painter.fillPath(path, QBrush(Qt::gray));
}

void SyllableDecorations::CalculateGeometry()
{
    QTextCursor cursor(m_text_edit->document());
    cursor.setPosition(m_start_index);
    const QRect start_rect = m_text_edit->cursorRect(cursor);
    cursor.setPosition(m_end_index);
    const QRect end_rect = m_text_edit->cursorRect(cursor);

    const int left = std::min(start_rect.left(), end_rect.left()) - SYLLABLE_MARKER_WIDTH;
    const int top = std::max(start_rect.bottom(), end_rect.bottom());
    const int width = std::abs(end_rect.left() - start_rect.left()) + SYLLABLE_MARKER_WIDTH;
    const int height = SYLLABLE_MARKER_HEIGHT + PROGRESS_LINE_HEIGHT;
    setGeometry(QRect(left, top, width, height));
}

LineTimingDecorations::LineTimingDecorations(KaraokeData::Line* line, int position,
                                             QPlainTextEdit* text_edit, QObject* parent)
    : QObject(parent), m_line(line), m_position(position)
{
    auto syllables = m_line->GetSyllables();
    m_syllables.reserve(syllables.size());
    int i = m_position + m_line->GetPrefix().size();
    for (KaraokeData::Syllable* syllable : syllables)
    {
        const int start_index = i;
        i += syllable->GetText().size();
        m_syllables.emplace_back(std::make_unique<SyllableDecorations>(
                        text_edit, start_index, i, syllable->GetStart(), syllable->GetEnd()));
    }
}

void LineTimingDecorations::Update(std::chrono::milliseconds time)
{
    const TimingState state = GetTimingState(time, m_line->GetStart(), m_line->GetEnd());
    if (state == m_state && state != TimingState::Playing)
        return;
    m_state = state;

    for (std::unique_ptr<SyllableDecorations>& syllable : m_syllables)
        syllable->Update(time, state != TimingState::Playing);
}

int LineTimingDecorations::GetPosition() const
{
    return m_position;
}
