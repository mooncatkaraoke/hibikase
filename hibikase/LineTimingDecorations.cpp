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

LineTimingDecorations::LineTimingDecorations(KaraokeData::Line* line, size_t position,
                                             QTextDocument* document, QObject* parent)
    : QObject(parent), m_line(line), m_position(position), m_document(document)
{
}

void LineTimingDecorations::Update(std::chrono::milliseconds time)
{
    QTextCharFormat active;
    active.setBackground(Qt::green);
    QTextCharFormat inactive;
    inactive.setBackground(Qt::white);

    QTextCursor cursor(m_document);
    const bool line_is_active = m_line->GetStart() <= time && m_line->GetEnd() >= time;
    if (!line_is_active)
    {
        cursor.setPosition(m_position, QTextCursor::MoveAnchor);
        cursor.setPosition(m_position + m_line->GetText().size(), QTextCursor::KeepAnchor);
        cursor.setCharFormat(inactive);
    }
    else
    {
        size_t i = m_position;
        for (KaraokeData::Syllable* syllable : m_line->GetSyllables())
        {
            cursor.setPosition(i, QTextCursor::MoveAnchor);
            i += syllable->GetText().size();
            cursor.setPosition(i, QTextCursor::KeepAnchor);
            const bool syllable_is_active = syllable->GetStart() <= time && syllable->GetEnd() >= time;
            cursor.setCharFormat(syllable_is_active ? active : inactive);
        }
    }
}
