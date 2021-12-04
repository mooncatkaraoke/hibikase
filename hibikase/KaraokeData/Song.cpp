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

#include "KaraokeData/ReadOnlySong.h"
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

void Line::Split(int split_position, ReadOnlyLine* first_out, ReadOnlyLine* second_out,
                 bool* syllable_boundary_at_split_point_out) const
{
    const QString prefix = GetPrefix();
    const auto syllables = GetSyllables();

    first_out->m_syllables.clear();
    second_out->m_syllables.clear();

    if (split_position <= prefix.size())
    {
        // Split inside prefix (or between prefix and first syllable)

        first_out->m_prefix = prefix.left(split_position);
        second_out->m_prefix = prefix.right(prefix.size() - split_position);

        second_out->m_syllables.reserve(syllables.size());
        for (const Syllable* syllable : syllables)
            second_out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(*syllable));

        *syllable_boundary_at_split_point_out = split_position == prefix.size();
    }
    else
    {
        first_out->m_prefix = prefix;
        second_out->m_prefix.clear();

        *syllable_boundary_at_split_point_out = true;

        int syllable_index = 0;
        int position = prefix.size();

        // Iterate through syllables before the split point
        for (; syllable_index < syllables.size(); ++syllable_index)
        {
            if (split_position == position)
            {
                // Split between two syllables
                break;
            }

            const Syllable* syllable = syllables[syllable_index];
            const QString text = syllable->GetText();
            const int next_position = position + text.size();

            if (split_position < next_position)
            {
                // Split in the middle of a syllable

                const QString first_text = text.left(split_position - position);
                const QString second_text = text.right(next_position - split_position);

                first_out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(
                        first_text, syllable->GetStart(), PLACEHOLDER_TIME));
                second_out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(
                        second_text, PLACEHOLDER_TIME, syllable->GetEnd()));

                *syllable_boundary_at_split_point_out = false;

                ++syllable_index;
                break;
            }
            else
            {
                // No split yet. Copy the current syllable and continue looping
                first_out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(*syllable));
            }

            position = next_position;
        }

        // Iterate through syllables after the split point
        for (; syllable_index < syllables.size(); ++syllable_index)
        {
            const Syllable* syllable = syllables[syllable_index];
            second_out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(*syllable));
        }
    }
}

void Line::Join(const Line& other, bool syllable_boundary_at_split_point, ReadOnlyLine* out) const
{
    const QString this_prefix = GetPrefix();
    const QString other_prefix = other.GetPrefix();

    const auto this_syllables = GetSyllables();
    const auto other_syllables = other.GetSyllables();

    out->m_prefix = this_prefix;
    out->m_syllables.clear();

    bool join_syllables = !syllable_boundary_at_split_point;

    int other_syllables_size = other_syllables.size();
    if (!other_prefix.isEmpty())
        ++other_syllables_size;
    if (other_syllables_size != 0 && join_syllables)
        --other_syllables_size;
    out->m_syllables.reserve(this_syllables.size() + other_syllables_size);

    // This function adds a syllable to out, optionally joining it with the previous syllable
    const auto process_other_syllable = [&out, &join_syllables](const Syllable& syllable) {
        if (join_syllables)
        {
            if (out->m_syllables.empty())
            {
                out->m_prefix += syllable.GetText();
            }
            else
            {
                ReadOnlySyllable* previous_syllable = out->m_syllables.back().get();
                previous_syllable->m_text += syllable.GetText();
                previous_syllable->m_end = syllable.GetEnd();
            }

            join_syllables = false;
        }
        else
        {
            out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(syllable));
        }
    };

    for (const Syllable* syllable : this_syllables)
        out->m_syllables.emplace_back(std::make_unique<ReadOnlySyllable>(*syllable));

    if (!other_prefix.isEmpty())
        process_other_syllable(ReadOnlySyllable(other_prefix, PLACEHOLDER_TIME, PLACEHOLDER_TIME));

    for (const Syllable* syllable : other_syllables)
        process_other_syllable(*syllable);

    // If we're supposed to join syllables, but `other` contains no syllables (and no prefix to
    // turn into a syllable), remove the end time of the last syllable from `this` (if one exists),
    // as if it had been joined with a syllable that has no text and no start or end times.
    // This function will most likely never be called with inputs that would make this edge case
    // matter, but in case it does happen, this is probably the most sensible thing to do.
    if (join_syllables && !out->m_syllables.empty())
        out->m_syllables.back()->SetEnd(PLACEHOLDER_TIME);
}

QString Song::GetText(int start_line, int end_line) const
{
    if (start_line >= end_line && end_line >= 0)
        return QString();

    // TODO: Performance cost of GetLines() copying pointers into a QVector?
    const QVector<const Line*> lines = GetLines();

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

QString Song::GetText() const
{
    return GetText(0, -1);
}

QVector<const Line*> Song::GetLines(SongPosition start, SongPosition end,
                                    ReadOnlyLine* first_line_first_half_out,
                                    ReadOnlyLine* first_line_last_half_out,
                                    ReadOnlyLine* last_line_first_half_out,
                                    ReadOnlyLine* last_line_last_half_out,
                                    bool* syllable_boundary_at_start_out,
                                    bool* syllable_boundary_at_end_out) const
{
    const QVector<const Line*> lines = GetLines();
    QVector<const Line*> result;

    if (start.line != end.line)
    {
        lines[start.line]->Split(start.position_in_line, first_line_first_half_out,
                                 first_line_last_half_out, syllable_boundary_at_start_out);
        result.push_back(first_line_last_half_out);

        for (int i = start.line + 1; i < end.line; ++i)
            result.push_back(lines[i]);

        lines[end.line]->Split(end.position_in_line, last_line_first_half_out,
                               last_line_last_half_out, syllable_boundary_at_end_out);
        result.push_back(last_line_first_half_out);
    }
    else
    {
        const Line* line = lines[start.line];

        line->Split(start.position_in_line, first_line_first_half_out,
                    first_line_last_half_out, syllable_boundary_at_start_out);
        first_line_last_half_out->Split(end.position_in_line - start.position_in_line,
                                        last_line_first_half_out, last_line_last_half_out,
                                        syllable_boundary_at_end_out);

        result.push_back(last_line_first_half_out);

        if (start.position_in_line == end.position_in_line)
            *syllable_boundary_at_end_out = *syllable_boundary_at_start_out;
    }

    return result;
}

void Song::ReplaceLines(SongPosition start, SongPosition end,
                        const QVector<const Line*>& replace_with,
                        const Line* first_line_first_half, const Line* last_line_last_half,
                        bool syllable_boundary_at_start, bool syllable_boundary_at_end)
{
    Q_ASSERT(!replace_with.empty());

    QVector<const Line*> result = replace_with;

    ReadOnlyLine new_first_line;
    const Line* old_first_line = replace_with.front();
    first_line_first_half->Join(*old_first_line, syllable_boundary_at_start, &new_first_line);
    result.front() = &new_first_line;

    ReadOnlyLine new_last_line;
    const Line* old_last_line = replace_with.size() > 1 ? replace_with.back() : &new_first_line;
    old_last_line->Join(*last_line_last_half, syllable_boundary_at_end, &new_last_line);
    result.back() = &new_last_line;

    ReplaceLines(start.line, end.line - start.line + 1, result);
}

std::unique_ptr<Song> Load(const QByteArray& data)
{
    std::unique_ptr<Song> vsqx = ParseVsqx(data);
    if (vsqx->IsValid())
        return vsqx;

    return std::make_unique<SoramimiSong>(data);
}

}
