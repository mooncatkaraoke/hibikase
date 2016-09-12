// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string>
#include <vector>

#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiMoonCatSong.h"

namespace KaraokeData
{

SoramimiMoonCatSyllable::SoramimiMoonCatSyllable(const std::string& text,
                                                 Centiseconds start, Centiseconds end)
    : m_text(text), m_start(start), m_end(end)
{
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const std::string& content)
    : m_raw_content(content)
{
    Deserialize();
}

SoramimiMoonCatLine::SoramimiMoonCatLine(const std::vector<Syllable*>& syllables)
{
    Serialize(syllables);
}

std::vector<Syllable*> SoramimiMoonCatLine::GetSyllables()
{
    std::vector<Syllable*> result;
    result.reserve(m_syllables.size());
    for (SoramimiMoonCatSyllable& syllable : m_syllables)
        result.push_back(&syllable);
    return result;
}

void SoramimiMoonCatLine::Serialize(const std::vector<Syllable*>& syllables)
{
    m_raw_content.clear();

    Centiseconds previous_time = Centiseconds::min();
    size_t last_character_of_previous_text = 0;
    for (const Syllable* syllable : syllables)
    {
        Centiseconds start = syllable->GetStart();
        if (previous_time != start)
        {
            // TODO: Encoding
            if (m_raw_content[last_character_of_previous_text] == ' ')
            {
                m_raw_content.erase(last_character_of_previous_text, 1);
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

    const std::string& content = m_raw_content;

    bool first_timecode = true;
    Centiseconds previous_time;
    size_t previous_index;

    // Without this check, the content.size() - 10 calculation below can underflow
    if (content.size() < 10)
      return;

    // Loops from first character to the last character that possibly can be
    // the start of a timecode (the tenth character from the end)
    for (size_t i = 0; i <= content.size() - 10; ++i) {
        // TODO: Encoding
        if (content[i] == '[' && content[i + 3] == ':' &&
                content[i + 6] == ':' && content[i + 9] == ']')
        {
            int minutes = DeserializeNumber(content, i + 1, 2);
            int seconds = DeserializeNumber(content, i + 4, 2);
            int centiseconds = DeserializeNumber(content, i + 7, 2);

            // Seconds are not supposed to be higher than 59. This implementation
            // treats a timecode like [00:76:02] as [01:16:02], which seems to be
            // consistent with Soramimi Karaoke, Soramimi Karaoke Tools and ECHO.
            // An alternative would be to treat such timecodes as invalid.
            if (minutes != -1 && seconds != -1 && centiseconds != -1)
            {
                Centiseconds time = Minutes(minutes) + Seconds(seconds) +
                                    Centiseconds(centiseconds);

                if (first_timecode)
                {
                    first_timecode = false;
                }
                else
                {
                    std::string text = content.substr(previous_index, i - previous_index);
                    bool empty = true;
                    for (char character : text)
                    {
                        if (character != ' ')
                            empty = false;
                    }
                    if (empty)
                        m_syllables.back().m_text += text;
                    else
                        m_syllables.emplace_back(text, previous_time, time);
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

std::string SoramimiMoonCatLine::SerializeTime(Centiseconds time)
{
    // This relies on minutes and seconds being integers
    Minutes minutes = std::chrono::duration_cast<Minutes>(time);
    Seconds seconds = std::chrono::duration_cast<Seconds>(time - minutes);
    Centiseconds centiseconds = time - minutes - seconds;

    return "[" + SerializeNumber(minutes.count(), 2) + ":" +
                 SerializeNumber(seconds.count(), 2) + ":" +
                 SerializeNumber(centiseconds.count(), 2) + "]";
}

std::string SoramimiMoonCatLine::SerializeNumber(int number, size_t digits)
{
    std::string result = std::to_string(number);
    if (result.size() < digits)
        result.insert(0, digits - result.size(), '0');
    return result;
}

int SoramimiMoonCatLine::DeserializeNumber(const std::string& string,
                                           size_t position, size_t digits)
{
    for (size_t i = 0; i < digits; ++i)
        if (!isdigit(string[position + i]))
            return -1;

    return std::stoi(string.substr(position, digits));
}

SoramimiMoonCatSong::SoramimiMoonCatSong(const std::vector<char>& data)
{
    std::string current_line;

    for (char character : data)
    {
        // TODO: Encodings
        // TODO: This might not be a good way to handle line endings
        if (character != '\n')
        {
            current_line += character;
        }
        else
        {
            if (!current_line.empty() && current_line.back() == '\r')
                current_line.pop_back();
            m_lines.emplace_back(current_line);
            current_line.clear();
        }
    }

    if (!current_line.empty())
        m_lines.emplace_back(current_line);
}

SoramimiMoonCatSong::SoramimiMoonCatSong(const std::vector<Line*>& lines)
{
    for (Line* line : lines)
        m_lines.emplace_back(line->GetSyllables());
}

std::string SoramimiMoonCatSong::GetRaw() const
{
    std::string result;
    size_t size = 0;
    // TODO: The user might want LF instead of CRLF
    static const std::string LINE_ENDING = "\r\n";

    for (const SoramimiMoonCatLine& line : m_lines)
        size += line.GetRaw().size() + LINE_ENDING.size();

    result.reserve(size);

    for (const SoramimiMoonCatLine& line : m_lines)
        result += line.GetRaw() + LINE_ENDING;

    return result;
}

std::vector<Line*> SoramimiMoonCatSong::GetLines()
{
    std::vector<Line*> result;
    result.reserve(m_lines.size());
    for (SoramimiMoonCatLine& line : m_lines)
        result.push_back(&line);
    return result;
}

void SoramimiMoonCatSong::AddLine(const std::vector<Syllable*>& syllables)
{
    m_lines.emplace_back(syllables);
}

}
