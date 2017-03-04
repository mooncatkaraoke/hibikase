// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <algorithm>
#include <chrono>
#include <cctype>

#include <QByteArray>
#include <QList>
#include <QString>
#include <QTextStream>
#include <QVector>

#include "Settings.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiMoonCatSong.h"

namespace KaraokeData
{

SoramimiMoonCatSyllable::SoramimiMoonCatSyllable()
{
}

SoramimiMoonCatSyllable::SoramimiMoonCatSyllable(const QString& text,
                                                 Centiseconds start, Centiseconds end)
    : m_text(text), m_start(start), m_end(end)
{
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const QString& content)
    : m_raw_content(content)
{
    Deserialize();
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const QVector<Syllable*>& syllables)
{
    Serialize(syllables);
}

QVector<Syllable*> SoramimiMoonCatLine::GetSyllables()
{
    QVector<Syllable*> result{};
    result.reserve(m_syllables.size());
    for (SoramimiMoonCatSyllable& syllable : m_syllables)
        result.push_back(&syllable);
    return result;
}

void SoramimiMoonCatLine::Serialize(const QVector<Syllable*>& syllables)
{
    m_raw_content.clear();

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
                // between the two timecode instead of before. This isn't
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

    Deserialize();
}

void SoramimiMoonCatLine::Deserialize()
{
    m_syllables.clear();
    m_start = Centiseconds::max();
    m_end = Centiseconds::min();

    bool first_timecode = true;
    Centiseconds previous_time;
    size_t previous_index;

    // Without this check, the m_raw_content.size() - 10 calculation below can underflow
    if (m_raw_content.size() < 10)
      return;

    // Loops from first character to the last character that possibly can be
    // the start of a timecode (the tenth character from the end)
    for (int i = 0; i <= m_raw_content.size() - 10; ++i) {
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
                    first_timecode = false;
                }
                else
                {
                    const QStringRef text(&m_raw_content, previous_index, i - previous_index);
                    const bool empty = text.count(' ') == text.size();
                    if (empty)
                        m_syllables.back().m_text += text;
                    else
                        m_syllables.append(SoramimiMoonCatSyllable(text.toString(), previous_time, time));
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
    // TODO: Handle text before the first timecode and after the last timecode
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
        m_lines.append(SoramimiMoonCatLine(stream.readLine()));
}

SoramimiMoonCatSong::SoramimiMoonCatSong(const QVector<Line*>& lines)
{
    for (Line* line : lines)
        m_lines.append(SoramimiMoonCatLine(line->GetSyllables()));
}

QString SoramimiMoonCatSong::GetRaw() const
{
    QString result;
    size_t size = 0;
    // TODO: The user might want LF instead of CRLF
    static const QString LINE_ENDING = "\r\n";

    for (const SoramimiMoonCatLine& line : m_lines)
        size += line.GetRaw().size() + LINE_ENDING.size();

    result.reserve(size);

    for (const SoramimiMoonCatLine& line : m_lines)
        result += line.GetRaw() + LINE_ENDING;

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
    for (SoramimiMoonCatLine& line : m_lines)
        result.push_back(&line);
    return result;
}

void SoramimiMoonCatSong::AddLine(const QVector<Syllable*>& syllables)
{
    m_lines.append(SoramimiMoonCatLine(syllables));
}

void SoramimiMoonCatSong::RemoveAllLines()
{
    m_lines.clear();
}

}
