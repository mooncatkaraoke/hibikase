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
#include "KaraokeData/SoramimiMoonCatSong.h"

namespace KaraokeData
{

static const QString PLACEHOLDER_TIMECODE = QStringLiteral("[99:59:99]");
static constexpr Centiseconds PLACEHOLDER_TIME = Centiseconds(99 * 60 * 100 + 59 * 100 + 99);

SoramimiMoonCatSyllable::SoramimiMoonCatSyllable(const QString& text,
                                                 Centiseconds start, Centiseconds end)
    : m_text(text), m_start(start), m_end(end)
{
}

void SoramimiMoonCatSyllable::SetText(const QString& text)
{
    m_text = text;
    emit Changed();
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const QString& content)
    : m_raw_content(content)
{
    Deserialize();
    BuildText();
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const QVector<Syllable*>& syllables, QString prefix)
    : m_prefix(prefix)
{
    Serialize(syllables);
    Deserialize();
    BuildText();
}

QVector<Syllable*> SoramimiMoonCatLine::GetSyllables()
{
    QVector<Syllable*> result{};
    result.reserve(m_syllables.size());
    for (std::unique_ptr<SoramimiMoonCatSyllable>& syllable : m_syllables)
        result.push_back(syllable.get());
    return result;
}

void SoramimiMoonCatLine::SetPrefix(const QString& text)
{
    m_prefix = text;
    Serialize();
    BuildText();
}

void SoramimiMoonCatLine::SetSyllableSplitPoints(QVector<int> split_points)
{
    m_raw_content = GetText();
    int characters_added = 0;

    for (int split_point : split_points)
    {
        m_raw_content.insert(split_point + characters_added, PLACEHOLDER_TIMECODE);
        characters_added += PLACEHOLDER_TIMECODE.size();
    }

    Deserialize();
}

void SoramimiMoonCatLine::Serialize()
{
    // TODO: Performance cost of GetSyllables() copying pointers into a QVector?
    Serialize(GetSyllables());
}

void SoramimiMoonCatLine::Serialize(const QVector<Syllable*>& syllables)
{
    m_raw_content.clear();

    m_raw_content += m_prefix;

    Centiseconds previous_time = Centiseconds::min();
    size_t last_character_of_previous_text = 0;
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

        m_raw_content += syllable->GetText();
        last_character_of_previous_text = m_raw_content.size() - 1;

        Centiseconds end = syllable->GetEnd();
        m_raw_content += SerializeTime(end);
        previous_time = end;
    }

    if (m_raw_content.endsWith(PLACEHOLDER_TIMECODE))
        m_raw_content.chop(PLACEHOLDER_TIMECODE.size());
}

void SoramimiMoonCatLine::Deserialize()
{
    m_syllables.clear();
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

void SoramimiMoonCatLine::AddSyllable(size_t start, size_t end,
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
        m_syllables.emplace_back(std::make_unique<SoramimiMoonCatSyllable>(
                                 text.toString(), start_time, end_time));
        connect(m_syllables.back().get(), &SoramimiMoonCatSyllable::Changed,
                this, [this] { Serialize(); });
        connect(m_syllables.back().get(), &SoramimiMoonCatSyllable::Changed,
                this, &SoramimiMoonCatLine::BuildText);
    }
}

QString SoramimiMoonCatLine::SerializeTime(Centiseconds time)
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

QString SoramimiMoonCatLine::SerializeNumber(int number, int digits)
{
    return QString("%1").arg(number, digits, 10, QChar('0'));
}

SoramimiMoonCatSong::SoramimiMoonCatSong(const QByteArray& data)
{
    QTextStream stream(data);
    stream.setCodec(Settings::GetLoadCodec(data));
    while (!stream.atEnd())
        m_lines.push_back(std::make_unique<SoramimiMoonCatLine>(stream.readLine()));
}

SoramimiMoonCatSong::SoramimiMoonCatSong(const QVector<Line*>& lines)
{
    for (Line* line : lines)
        m_lines.push_back(std::make_unique<SoramimiMoonCatLine>(
                          line->GetSyllables(), line->GetPrefix()));
}

QString SoramimiMoonCatSong::GetRaw() const
{
    QString result;
    size_t size = 0;
    // TODO: The user might want LF instead of CRLF
    static const QString LINE_ENDING = "\r\n";

    for (const std::unique_ptr<SoramimiMoonCatLine>& line : m_lines)
        size += line->GetRaw().size() + LINE_ENDING.size();

    result.reserve(size);

    for (const std::unique_ptr<SoramimiMoonCatLine>& line : m_lines)
        result += line->GetRaw() + LINE_ENDING;

    return result;
}

QByteArray SoramimiMoonCatSong::GetRawBytes() const
{
    return Settings::GetSaveCodec()->fromUnicode(GetRaw());
}

QVector<Line*> SoramimiMoonCatSong::GetLines()
{
    QVector<Line*> result;
    result.reserve(m_lines.size());
    for (std::unique_ptr<SoramimiMoonCatLine>& line : m_lines)
        result.push_back(line.get());
    return result;
}

void SoramimiMoonCatSong::AddLine(const QVector<Syllable*>& syllables, QString prefix)
{
    m_lines.push_back(std::make_unique<SoramimiMoonCatLine>(syllables, prefix));
}

void SoramimiMoonCatSong::RemoveAllLines()
{
    m_lines.clear();
}

}
