// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include <chrono>
#include <map>
#include <memory>

#include <QByteArray>
#include <QString>
#include <QXmlStreamReader>

#include "KaraokeData/ReadOnlySong.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/TempoMap.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

enum VsqVersion
{
    VSQ3,
    VSQ4
};

std::unique_ptr<Song> ParseVsqx(const QByteArray& data)
{
    QXmlStreamReader reader(data);

    if (!reader.readNextStartElement())
        return nullptr;

    VsqVersion version;
    if (reader.name() == QStringLiteral("vsq3"))
        version = VSQ3;
    else if (reader.name() == QStringLiteral("vsq4"))
        version = VSQ4;
    else
        return nullptr;

    static const QString numeratorName{version == VSQ3 ? QStringLiteral("nume") : QStringLiteral("nu")};
    static const QString bpmName{version == VSQ3 ? QStringLiteral("bpm") : QStringLiteral("v")};
    static const QString musicalPartName{version == VSQ3 ? QStringLiteral("musicalPart") : QStringLiteral("vsPart")};
    static const QString posTickName{version == VSQ3 ? QStringLiteral("posTick") : QStringLiteral("t")};
    static const QString durTickName{version == VSQ3 ? QStringLiteral("durTick") : QStringLiteral("dur")};
    static const QString lyricName{version == VSQ3 ? QStringLiteral("lyric") : QStringLiteral("y")};

    std::unique_ptr<ReadOnlySong> song = std::make_unique<ReadOnlySong>();
    TempoMap tempo_map;
    DoubleCentiseconds offset = DoubleCentiseconds::zero();

    while (reader.readNextStartElement())
    {
        QStringRef name1 = reader.name();
        if (name1 == QStringLiteral("masterTrack"))
        {
            int resolution = 0;
            int pre_measure = 0;
            int time_signature_numerator = 0;

            while (reader.readNextStartElement())
            {
                QStringRef name2 = reader.name();
                if (name2 == QStringLiteral("resolution"))
                {
                    resolution = reader.readElementText().toInt();
                }
                else if (name2 == QStringLiteral("preMeasure"))
                {
                    pre_measure = reader.readElementText().toInt();
                }
                else if (name2 == QStringLiteral("timeSig"))
                {
                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == numeratorName)
                            time_signature_numerator = reader.readElementText().toInt();
                        else
                            reader.skipCurrentElement();
                    }
                }
                else if (name2 == QStringLiteral("tempo"))
                {
                    int position_ticks = -1;
                    int bpm_times_100 = -1;

                    while (reader.readNextStartElement())
                    {
                        if (reader.name() == posTickName)
                            position_ticks = reader.readElementText().toInt();
                        else if (reader.name() == bpmName)
                            bpm_times_100 = reader.readElementText().toInt();
                        else
                            reader.skipCurrentElement();
                    }

                    if (resolution != 0 && position_ticks != -1 && bpm_times_100 != -1)
                    {
                        const DoubleCentiseconds hundred_minutes = std::chrono::minutes(100);
                        const DoubleCentiseconds tick_duration =
                                hundred_minutes / (static_cast<double>(bpm_times_100) * resolution);
                        tempo_map.AddTempoEntry(position_ticks, tick_duration);
                    }
                }
                else
                {
                    reader.skipCurrentElement();
                }
            }

            if (time_signature_numerator != 0 && resolution != 0)
                offset = tempo_map.TicksToTime(time_signature_numerator * pre_measure * resolution);
        }
        else if (!tempo_map.empty() && name1 == QStringLiteral("vsTrack"))
        {
            while (reader.readNextStartElement())
            {
                if (reader.name() == musicalPartName)
                {
                    int part_start_ticks = 0;
                    song->m_lines.emplace_back(std::make_unique<ReadOnlyLine>());

                    while (reader.readNextStartElement())
                    {
                        QStringRef name2 = reader.name();
                        if (name2 == posTickName)
                        {
                            part_start_ticks = reader.readElementText().toInt();
                        }
                        else if (name2 == QStringLiteral("note"))
                        {
                            int position_ticks = -1;
                            int duration_ticks = -1;
                            QString lyric;

                            while (reader.readNextStartElement())
                            {
                                QStringRef name3 = reader.name();
                                if (name3 == posTickName)
                                    position_ticks = part_start_ticks + reader.readElementText().toInt();
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
                                Centiseconds start = std::chrono::duration_cast<Centiseconds>(
                                            tempo_map.TicksToTime(position_ticks) - offset);
                                Centiseconds end = std::chrono::duration_cast<Centiseconds>(
                                            tempo_map.TicksToTime(position_ticks + duration_ticks) - offset);
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

    if (song->m_lines.empty())
        return nullptr;

    return song;
}

}
