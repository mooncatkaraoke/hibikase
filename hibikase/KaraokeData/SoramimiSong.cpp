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

#include <algorithm>
#include <chrono>
#include <cctype>
#include <memory>
#include <utility>
#include <vector>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "Settings.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiSong.h"

namespace KaraokeData
{

static const QString PLACEHOLDER_TIMECODE = QStringLiteral("[99:59:99]");
static constexpr Centiseconds PLACEHOLDER_TIME = Centiseconds(99 * 60 * 100 + 59 * 100 + 99);

// TODO: The user might want LF instead of CRLF
static const QString LINE_ENDING = "\r\n";

SoramimiSyllable::SoramimiSyllable(const QString& text, Centiseconds start, Centiseconds end)
    : m_text(text), m_start(start), m_end(end)
{
}

void SoramimiSyllable::SetText(const QString& text)
{
    m_text = text;
    emit Changed();
}

SoramimiLine::SoramimiLine(const QString& content)
    : m_raw_content(content)
{
    Deserialize();
    BuildText();
}

SoramimiLine::SoramimiLine(const QVector<Syllable*>& syllables, QString prefix)
    : m_prefix(prefix)
{
    Serialize(syllables);
    Deserialize();
    BuildText();
}

QVector<Syllable*> SoramimiLine::GetSyllables()
{
    QVector<Syllable*> result{};
    result.reserve(m_syllables.size());
    for (std::unique_ptr<SoramimiSyllable>& syllable : m_syllables)
        result.push_back(syllable.get());
    return result;
}

void SoramimiLine::SetPrefix(const QString& text)
{
    m_prefix = text;
    Serialize();
    BuildText();

    emit Changed();
}

void SoramimiLine::SetRaw(QString raw)
{
    m_raw_content = raw;
    Deserialize();
    BuildText();

    emit Changed();
}

void SoramimiLine::SetSyllableSplitPoints(QVector<int> split_points)
{
    m_raw_content = GetText();
    int characters_added = 0;

    for (int split_point : split_points)
    {
        m_raw_content.insert(split_point + characters_added, PLACEHOLDER_TIMECODE);
        characters_added += PLACEHOLDER_TIMECODE.size();
    }

    Deserialize();

    emit Changed();
}

int SoramimiLine::PositionFromRaw(int raw_position) const
{
    if (raw_position <= m_prefix.size())
        return raw_position;

    size_t syllable_number = 0;
    int current_position = m_prefix.size();
    for (; syllable_number < m_raw_syllable_positions.size(); ++syllable_number)
    {
        const int syllable_size = m_syllables[syllable_number]->GetText().size();
        const int raw_syllable_position = m_raw_syllable_positions[syllable_number];

        if (raw_position <= raw_syllable_position + syllable_size)
        {
            const int position_in_syllable = std::max(0, raw_position - raw_syllable_position);
            return current_position + position_in_syllable;
        }
        current_position += syllable_size;
    }
    return current_position;
}

int SoramimiLine::PositionToRaw(int position) const
{
    if (position <= m_prefix.size())
        return position;

    size_t syllable_number = 0;
    int current_position = m_prefix.size();
    for (const std::unique_ptr<SoramimiSyllable>& syllable : m_syllables)
    {
        if (position <= current_position)
        {
            const int position_in_syllable = std::max(0, position - current_position);
            return m_raw_syllable_positions[syllable_number] + position_in_syllable;
        }
        syllable_number++;
        current_position += syllable->GetText().size();
    }
    return current_position;
}

void SoramimiLine::Serialize()
{
    // TODO: Performance cost of GetSyllables() copying pointers into a QVector?
    Serialize(GetSyllables());
}

void SoramimiLine::Serialize(const QVector<Syllable*>& syllables)
{
    m_raw_content.clear();
    m_raw_syllable_positions.clear();

    m_raw_content += m_prefix;

    Centiseconds previous_time = Centiseconds::min();
    int last_character_of_previous_text = 0;
    for (const Syllable* syllable : syllables)
    {
        Centiseconds start = syllable->GetStart();
        if (previous_time != start)
        {
            if (m_raw_content[last_character_of_previous_text] == ' ')
            {
                // If the previous syllable ended with a space, put the space
                // between the two timecodes instead of before. This isn't
                // strictly required, but it's common practice because Soramimi
                // Karaoke Tools doesn't handle adjacent timecodes perfectly.
                m_raw_content.remove(last_character_of_previous_text, 1);
                m_raw_content += ' ';
            }
            m_raw_content += SerializeTime(start);
        }

        m_raw_syllable_positions.push_back(m_raw_content.size());
        m_raw_content += syllable->GetText();
        last_character_of_previous_text = m_raw_content.size() - 1;

        Centiseconds end = syllable->GetEnd();
        m_raw_content += SerializeTime(end);
        previous_time = end;
    }

    if (m_raw_content.endsWith(PLACEHOLDER_TIMECODE))
        m_raw_content.chop(PLACEHOLDER_TIMECODE.size());
}

void SoramimiLine::Deserialize()
{
    m_syllables.clear();
    m_raw_syllable_positions.clear();
    m_start = Centiseconds::max();
    m_end = Centiseconds::min();
    m_prefix.clear();

    bool first_timecode = true;
    Centiseconds previous_time;
    size_t previous_index = 0;

    // Loops from first character to the last character that possibly can be
    // the start of a timecode (the tenth character from the end)
    for (int i = 0; i <= m_raw_content.size() - 10; ++i)
    {
        if (m_raw_content[i] == '[' && m_raw_content[i + 3] == ':' &&
            m_raw_content[i + 6] == ':' && m_raw_content[i + 9] == ']')
        {
            bool minutesOk;
            const int minutes = QStringRef(&m_raw_content, i + 1, 2).toInt(&minutesOk, 10);
            bool secondsOk;
            const int seconds = QStringRef(&m_raw_content, i + 4, 2).toInt(&secondsOk, 10);
            bool centisecondsOk;
            const int centiseconds = QStringRef(&m_raw_content, i + 7, 2).toInt(&centisecondsOk, 10);

            // Seconds are not supposed to be higher than 59. This implementation
            // treats a timecode like [00:76:02] as [01:16:02], which seems to be
            // consistent with Soramimi Karaoke, Soramimi Karaoke Tools and ECHO.
            // An alternative would be to treat such timecodes as invalid.
            if (minutesOk && secondsOk && centisecondsOk)
            {
                const Centiseconds time = Minutes(minutes) + Seconds(seconds) +
                                          Centiseconds(centiseconds);

                if (first_timecode)
                {
                    m_prefix = m_raw_content.left(i);
                    first_timecode = false;
                }
                else
                {
                    AddSyllable(previous_index, i, previous_time, time);
                }

                previous_index = i + 10;
                previous_time = time;

                m_start = std::min(time, m_start);
                m_end = std::max(time, m_end);

                // Skip unnecessary loop iterations
                i += 9;
            }
        }
    }

    // Handle the case where there's text that isn't succeeded by a timecode
    if (first_timecode)
        m_prefix = m_raw_content;
    else
        AddSyllable(previous_index, m_raw_content.size(), previous_time, PLACEHOLDER_TIME);
}

void SoramimiLine::AddSyllable(size_t start, size_t end,
                               Centiseconds start_time, Centiseconds end_time)
{
    const QStringRef text(&m_raw_content, start, end - start);
    const bool empty = text.count(' ') == text.size();
    if (empty)
    {
        if (!m_syllables.empty())
            m_syllables.back()->m_text += text;
    }
    else
    {
        m_raw_syllable_positions.push_back(start);
        m_syllables.emplace_back(std::make_unique<SoramimiSyllable>(
                                 text.toString(), start_time, end_time));
        connect(m_syllables.back().get(), &SoramimiSyllable::Changed, this, [this] {
            Serialize();
            BuildText();
            emit Changed();
        });
    }
}

QString SoramimiLine::SerializeTime(Centiseconds time)
{
    // This relies on minutes and seconds being integers
    Minutes minutes = std::chrono::duration_cast<Minutes>(time);
    Seconds seconds = std::chrono::duration_cast<Seconds>(time - minutes);
    Centiseconds centiseconds = time - minutes - seconds;

    // TODO: What if minutes >= 100?
    return "[" + SerializeNumber(minutes.count(), 2) + ":" +
                 SerializeNumber(seconds.count(), 2) + ":" +
                 SerializeNumber(centiseconds.count(), 2) + "]";
}

QString SoramimiLine::SerializeNumber(int number, int digits)
{
    return QString("%1").arg(number, digits, 10, QChar('0'));
}

SoramimiSong::SoramimiSong(const QByteArray& data)
{
    QTextStream stream(data);
    stream.setCodec(Settings::GetLoadCodec(data));
    while (!stream.atEnd())
        m_lines.push_back(SetUpLine(std::make_unique<SoramimiLine>(stream.readLine())));
}

SoramimiSong::SoramimiSong(const QVector<Line*>& lines)
{
    for (Line* line : lines)
    {
        m_lines.push_back(SetUpLine(std::make_unique<SoramimiLine>(
                                    line->GetSyllables(), line->GetPrefix())));
    }
}

QString SoramimiSong::GetRaw() const
{
    QString result;
    size_t size = 0;

    for (const std::unique_ptr<SoramimiLine>& line : m_lines)
        size += line->GetRaw().size() + LINE_ENDING.size();

    result.reserve(size);

    for (const std::unique_ptr<SoramimiLine>& line : m_lines)
        result += line->GetRaw() + LINE_ENDING;
    if (!result.isEmpty())
        result.chop(LINE_ENDING.size());

    return result;
}

QByteArray SoramimiSong::GetRawBytes() const
{
    return Settings::GetSaveCodec()->fromUnicode(GetRaw());
}

QVector<Line*> SoramimiSong::GetLines()
{
    QVector<Line*> result;
    result.reserve(m_lines.size());
    for (std::unique_ptr<SoramimiLine>& line : m_lines)
        result.push_back(line.get());
    return result;
}

void SoramimiSong::AddLine(const QVector<Syllable*>& syllables, QString prefix)
{
    m_lines.push_back(SetUpLine(std::make_unique<SoramimiLine>(syllables, prefix)));
    emit LinesChanged(m_lines.size() - 1, 1, 1);
}

void SoramimiSong::RemoveAllLines()
{
    const size_t size = m_lines.size();
    m_lines.clear();
    emit LinesChanged(0, size, 0);
}

bool SoramimiSong::SupportsPositionConversion() const
{
    return true;
}

SongPosition SoramimiSong::RawPositionFromRaw(int raw_position) const
{
    int line_number = 0;
    int current_raw_pos = 0;
    for (const std::unique_ptr<SoramimiLine>& line : m_lines)
    {
        const int new_raw_pos = current_raw_pos + line->GetRaw().size() + 1;
        if (new_raw_pos > raw_position)
            break;

        line_number++;
        current_raw_pos = new_raw_pos;
    }

    const int position_in_line = raw_position - current_raw_pos;
    return {line_number, position_in_line};
}

SongPosition SoramimiSong::PositionFromRaw(int raw_position) const
{
    int line_number = 0;
    int current_raw_pos = 0;
    for (const std::unique_ptr<SoramimiLine>& line : m_lines)
    {
        const int new_raw_pos = current_raw_pos + line->GetRaw().size() + 1;
        if (new_raw_pos > raw_position)
            break;

        line_number++;
        current_raw_pos = new_raw_pos;
    }

    const int position_in_line = line_number >= m_lines.size() ? 0 :
                                 m_lines[line_number]->PositionFromRaw(raw_position - current_raw_pos);
    return {line_number, position_in_line};
}

int SoramimiSong::PositionToRaw(SongPosition position) const
{
    int raw_position = 0;
    for (size_t i = 0; i < position.line && i < m_lines.size(); ++i)
        raw_position += m_lines[i]->GetRaw().size() + 1;
    return position.line == m_lines.size() ? raw_position :
            raw_position + m_lines[position.line]->PositionToRaw(position.position_in_line);
}

void SoramimiSong::UpdateRawText(int position, int chars_to_remove, QStringRef replace_with)
{
    const SongPosition start = RawPositionFromRaw(position);
    const SongPosition end = RawPositionFromRaw(position + chars_to_remove);

    const QVector<QStringRef> new_lines = replace_with.split("\n");

    int lines_added_at_end = 0;
    while (end.line >= m_lines.size())
    {
        m_lines.emplace_back(SetUpLine(std::make_unique<SoramimiLine>(QString())));
        lines_added_at_end++;
    }

    const QString first_line = m_lines[start.line]->GetRaw();
    const QStringRef before = first_line.leftRef(start.position_in_line);
    const QString last_line = m_lines[end.line]->GetRaw();
    const QStringRef after = last_line.rightRef(last_line.size() - end.position_in_line);

    const int second_line = std::min<int>(start.line + 1, m_lines.size());
    const int old_lines_count = end.line - start.line + 1;
    const int new_lines_count = new_lines.size();
    auto replace_it = m_lines.begin() + second_line;
    int lines_to_skip = 0;
    if (old_lines_count > new_lines_count)
    {
        m_lines.erase(replace_it, replace_it + (old_lines_count - new_lines_count));
    }
    else if (old_lines_count < new_lines_count)
    {
        std::vector<std::unique_ptr<SoramimiLine>> lines_to_insert(new_lines_count - old_lines_count);
        auto it = new_lines.begin();
        for (auto& line_to_insert : lines_to_insert)
        {
            ++it;
            line_to_insert = SetUpLine(std::make_unique<SoramimiLine>(
                                       it != new_lines.end() ? it->toString() : QString()));
        }

        m_lines.insert(replace_it, std::make_move_iterator(lines_to_insert.begin()),
                                   std::make_move_iterator(lines_to_insert.end()));
        lines_to_skip = lines_to_insert.size();
    }

    m_updates_disabled = true;
    if (new_lines_count == 1)
    {
        m_lines[start.line]->SetRaw(before + replace_with + after);
    }
    else
    {
        m_lines[start.line]->SetRaw(before + new_lines.front());
        for (int i = 1 + lines_to_skip; i < new_lines.size() - 1; ++i)
            m_lines[start.line + i]->SetRaw(new_lines[i].toString());
        m_lines[start.line + new_lines.size() - 1]->SetRaw(new_lines.back() + after);
    }
    m_updates_disabled = false;

    emit LinesChanged(start.line, old_lines_count - lines_added_at_end, new_lines_count);
}

std::unique_ptr<SoramimiLine> SoramimiSong::SetUpLine(std::unique_ptr<SoramimiLine> line)
{
    const SoramimiLine* line_ptr = line.get();
    connect(line_ptr, &SoramimiLine::Changed, this, [this, line_ptr] {
        EmitLineChanged(line_ptr);
    });
    return line;
}

void SoramimiSong::EmitLineChanged(const SoramimiLine* line)
{
    if (m_updates_disabled)
        return;

    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        if (m_lines[i].get() == line)
        {
            emit LinesChanged(i, 1, 1);
            break;
        }
    }
}

}
