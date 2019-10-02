// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// As an additional permission for this file only, you can (at your
// option) instead use this file under the terms of CC0.
// <http://creativecommons.org/publicdomain/zero/1.0/>

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <memory>

#include <QByteArray>
#include <QString>

#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiSong.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

QString Line::GetText() const
{
    return m_text;
}

void Line::BuildText()
{
    // TODO: Performance cost of GetSyllables() copying pointers into a QVector?
    const QVector<Syllable*> syllables = GetSyllables();

    int size = GetPrefix().size();
    for (const Syllable* syllable : syllables)
        size += syllable->GetText().size();

    m_text.clear();
    m_text.reserve(size);
    m_text += GetPrefix();
    for (const Syllable* syllable : syllables)
        m_text += syllable->GetText();
}

QString Song::GetText(int start_line, int end_line)
{
    if (start_line >= end_line && end_line >= 0)
        return QString();

    // TODO: Performance cost of GetLines() copying pointers into a QVector?
    const QVector<Line*> lines = GetLines();

    // Small hack for performance: Let the parameter-less version of GetText specify "all lines"
    // without having to call GetLines() to find out how many lines there are
    if (end_line < 0)
        end_line = lines.size();

    int size = 0;
    for (int i = start_line; i < end_line; ++i)
        size += lines[i]->GetText().size() + sizeof('\n');

    QString text;
    text.reserve(size);

    for (int i = start_line; i < end_line; ++i)
    {
        text += lines[i]->GetText();
        text += '\n';
    }
    text.chop(sizeof('\n'));

    return text;
}

QString Song::GetText()
{
    return GetText(0, -1);
}

std::unique_ptr<Song> Load(const QByteArray& data)
{
    std::unique_ptr<Song> vsqx = ParseVsqx(data);
    if (vsqx->IsValid())
        return vsqx;

    return std::make_unique<SoramimiSong>(data);
}

}
