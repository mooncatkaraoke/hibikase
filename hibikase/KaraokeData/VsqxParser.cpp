// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <QXmlStreamReader>
#include <QByteArray>

#include "KaraokeData/ReadOnlySong.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

std::unique_ptr<Song> ParseVsqx(const std::vector<char>& data)
{
    std::unique_ptr<ReadOnlySong> song = std::make_unique<ReadOnlySong>();
    song->m_valid = false;

    // TODO: This code isn't supposed to require Qt
    QByteArray byte_array = QByteArray::fromRawData(data.data(), data.size());
    QXmlStreamReader reader(byte_array);

    if (!reader.readNextStartElement())
        return song;

    if (reader.name() != QStringLiteral("vsq3"))
        return song;

    std::chrono::nanoseconds tick_duration(0);
    int resolution = 0;
    int bpm_times_100 = 0;

    while (reader.readNextStartElement())
    {
        QStringRef name1 = reader.name();
        if (name1 == QStringLiteral("masterTrack"))
        {
            while (reader.readNextStartElement())
            {
                QStringRef name2 = reader.name();
                if (name2 == QStringLiteral("resolution"))
                {
                    resolution = reader.readElementText().toInt();
                }
                else if (name2 == QStringLiteral("tempo"))
                {
                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == QStringLiteral("bpm"))
                            bpm_times_100 = reader.readElementText().toInt();
                        else
                            reader.skipCurrentElement();
                    }
                }
                else
                {
                    reader.skipCurrentElement();
                }
            }
            if (resolution != 0 && bpm_times_100 != 0)
            {
                tick_duration = std::chrono::minutes(100);
                tick_duration /= bpm_times_100;
                tick_duration /= resolution;
            }
        }
        else if (tick_duration != tick_duration.zero() &&
                 name1 == QStringLiteral("vsTrack"))
        {
            while (reader.readNextStartElement())
            {
                if (reader.name() == QStringLiteral("musicalPart"))
                {
                    song->m_lines.emplace_back();

                    while (reader.readNextStartElement())
                    {
                        QStringRef name2 = reader.name();
                        if (name2 == QStringLiteral("note"))
                        {
                            int position_ticks = -1;
                            int duration_ticks = -1;
                            QString lyric;

                            while (reader.readNextStartElement())
                            {
                                QStringRef name3 = reader.name();
                                if (name3 == QStringLiteral("posTick"))
                                    position_ticks = reader.readElementText().toInt();
                                else if (name3 == QStringLiteral("durTick"))
                                    duration_ticks = reader.readElementText().toInt();
                                else if (name3 == QStringLiteral("lyric"))
                                    lyric = reader.readElementText();
                                else
                                    reader.skipCurrentElement();
                            }

                            if (position_ticks != -1 && duration_ticks != -1 && !lyric.isEmpty())
                            {
                                std::string text = lyric.toUtf8().constData();
                                if (text.back() == '-')
                                {
                                    if (text.size() == 1)
                                        text.back() = '/';
                                    else
                                        text.pop_back();
                                }
                                else
                                {
                                    text += ' ';
                                }
                                auto start = std::chrono::duration_cast<Centiseconds>(
                                            tick_duration * position_ticks);
                                auto end = std::chrono::duration_cast<Centiseconds>(
                                            tick_duration * (position_ticks + duration_ticks));
                                song->m_lines.back().m_syllables.emplace_back(text, start, end);
                            }
                        }
                        else
                        {
                            reader.skipCurrentElement();
                        }
                    }

                    std::string& last_text = song->m_lines.back().m_syllables.back().m_text;
                    if (!last_text.empty() && last_text[last_text.size() - 1] == ' ')
                        last_text.pop_back();
                }
                else
                {
                    reader.skipCurrentElement();
                }
            }
        }
        else
        {
            reader.skipCurrentElement();
        }
    }

    song->m_valid = !song->m_lines.empty();

    return song;
}

}
