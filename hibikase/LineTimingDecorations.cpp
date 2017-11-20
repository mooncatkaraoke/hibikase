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

#include <chrono>

#include <QObject>
#include <QTextCursor>
#include <QTextDocument>

#include "KaraokeData/Song.h"

#include "LineTimingDecorations.h"

static QTextCharFormat ActiveColor()
{
    QTextCharFormat color;
    color.setBackground(Qt::green);
    return color;
}

static QTextCharFormat InactiveColor()
{
    QTextCharFormat color;
    color.setBackground(Qt::white);
    return color;
}

LineTimingDecorations::SyllableDecorations::SyllableDecorations(size_t start_index,
        size_t end_index, Milliseconds start_time, Milliseconds end_time)
    : m_start_index(start_index), m_end_index(end_index),
      m_start_time(start_time), m_end_time(end_time)
{
}

void LineTimingDecorations::SyllableDecorations::Update(Milliseconds time, QTextDocument* document)
{
    const bool is_active = m_start_time <= time && m_end_time >= time;
    if (is_active == m_was_active)
        return;
    m_was_active = is_active;

    QTextCursor cursor(document);
    cursor.setPosition(m_start_index, QTextCursor::MoveAnchor);
    cursor.setPosition(m_end_index, QTextCursor::KeepAnchor);
    cursor.setCharFormat(is_active ? ActiveColor() : InactiveColor());
}

LineTimingDecorations::LineTimingDecorations(KaraokeData::Line* line, size_t position,
                                             QTextDocument* document, QObject* parent)
    : QObject(parent), m_line(line), m_position(position), m_document(document)
{
    auto syllables = m_line->GetSyllables();
    m_syllables.reserve(syllables.size());
    size_t i = m_position;
    for (KaraokeData::Syllable* syllable : syllables)
    {
        const size_t start_index = i;
        i += syllable->GetText().size();
        m_syllables.emplace_back(start_index, i, syllable->GetStart(), syllable->GetEnd());
    }
}

void LineTimingDecorations::Update(Milliseconds time)
{
    const bool is_active = m_line->GetStart() <= time && m_line->GetEnd() >= time;
    if (!is_active && !m_was_active)
        return;
    m_was_active = is_active;

    for (SyllableDecorations& syllable : m_syllables)
        syllable.Update(time, m_document);
}
