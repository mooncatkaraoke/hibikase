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
#include <vector>

#include <QObject>
#include <QTextDocument>

#include "KaraokeData/Song.h"

class LineTimingDecorations final : public QObject
{
    Q_OBJECT

    using Milliseconds = std::chrono::milliseconds;

    class SyllableDecorations final
    {
    public:
        explicit SyllableDecorations(size_t start_index, size_t end_index,
                                     Milliseconds start_time, Milliseconds end_time);

        void Update(Milliseconds time, QTextDocument* document);

    private:
        const size_t m_start_index;
        const size_t m_end_index;
        const Milliseconds m_start_time;
        const Milliseconds m_end_time;
        bool m_was_active = false;
    };

public:
    LineTimingDecorations(KaraokeData::Line* line, size_t position,
                          QTextDocument* document, QObject* parent = nullptr);

    void Update(Milliseconds time);

private:
    std::vector<SyllableDecorations> m_syllables;
    // TODO: Use a const reference instead
    KaraokeData::Line* m_line;
    size_t m_position;
    QTextDocument* m_document;
    bool m_was_active = false;
};
