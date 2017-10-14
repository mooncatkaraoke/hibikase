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

#include <chrono>
#include <memory>

#include <QByteArray>
#include <QString>
#include <QXmlStreamReader>

#include "KaraokeData/ReadOnlySong.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

enum VsqVersion {
    VSQ3,
    VSQ4
};

std::unique_ptr<Song> ParseVsqx(const QByteArray& data)
{
    std::unique_ptr<ReadOnlySong> song = std::make_unique<ReadOnlySong>();
    song->m_valid = false;

    QXmlStreamReader reader(data);

    if (!reader.readNextStartElement())
        return song;

    VsqVersion version;
    if (reader.name() == QStringLiteral("vsq3"))
        version = VSQ3;
    else if (reader.name() == QStringLiteral("vsq4"))
        version = VSQ4;
    else
        return song;

    static const QString bpmName{version == VSQ3 ? QStringLiteral("bpm") : QStringLiteral("v")};
    static const QString musicalPartName{version == VSQ3 ? QStringLiteral("musicalPart") : QStringLiteral("vsPart")};
    static const QString posTickName{version == VSQ3 ? QStringLiteral("posTick") : QStringLiteral("t")};
    static const QString durTickName{version == VSQ3 ? QStringLiteral("durTick") : QStringLiteral("dur")};
    static const QString lyricName{version == VSQ3 ? QStringLiteral("lyric") : QStringLiteral("y")};

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
                        if (reader.name() == bpmName)
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
                if (reader.name() == musicalPartName)
                {
                    song->m_lines.emplace_back(std::make_unique<ReadOnlyLine>());

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
                                if (name3 == posTickName)
                                    position_ticks = reader.readElementText().toInt();
                                else if (name3 == durTickName)
                                    duration_ticks = reader.readElementText().toInt();
                                else if (name3 == lyricName)
                                    lyric = reader.readElementText();
                                else
                                    reader.skipCurrentElement();
                            }

                            if (position_ticks != -1 && duration_ticks != -1 && !lyric.isEmpty())
                            {
                                if (lyric.endsWith('-'))
                                {
                                    lyric.chop(1);
                                    if (lyric.isEmpty())
                                        lyric.append('/');
                                }
                                else
                                {
                                    lyric.append(' ');
                                }
                                auto start = std::chrono::duration_cast<Centiseconds>(
                                            tick_duration * position_ticks);
                                auto end = std::chrono::duration_cast<Centiseconds>(
                                            tick_duration * (position_ticks + duration_ticks));
                                song->m_lines.back()->m_syllables.emplace_back(
                                            std::make_unique<ReadOnlySyllable>(lyric, start, end));
                            }
                        }
                        else
                        {
                            reader.skipCurrentElement();
                        }
                    }

                    QString& last_lyric = song->m_lines.back()->m_syllables.back()->m_text;
                    if (last_lyric.endsWith(' '))
                        last_lyric.chop(1);
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
