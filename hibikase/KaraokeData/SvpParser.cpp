// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include "KaraokeData/SvpParser.h"

#include <chrono>
#include <memory>
#include <ratio>

#include <QByteArray>
#include <QChar>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "KaraokeData/ReadOnlySong.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/TempoMap.h"

namespace KaraokeData
{

static constexpr qint64 TICK_RESOLUTION = 16000;
using TimeConversionRatio = std::ratio_divide<std::nano, std::centi>;
static_assert(TimeConversionRatio::num == 1, "Unexpected TimeConversionRatio numerator");
static constexpr qint64 TIME_CONVERSION_DIVISOR = TICK_RESOLUTION * TimeConversionRatio::den;

static long long ToLongLong(const QJsonValue& value)
{
    return value.toDouble();
}

std::unique_ptr<Song> ParseSvp(const QByteArray& data)
{
    const QJsonDocument json = QJsonDocument::fromJson(data);
    if (json.isNull())
        return nullptr;

    QJsonObject root = json.object();

    QJsonValue sample_rate_value = root["renderConfig"].toObject()["sampleRate"];
    if (!sample_rate_value.isDouble())
        return nullptr;
    const qint64 sample_rate = ToLongLong(sample_rate_value);
    if (sample_rate == 0)
        return nullptr;

    TempoMap tempo_map;
    const QJsonArray tempo = root["time"].toObject()["tempo"].toArray();
    for (const auto& tempo_entry : tempo)
    {
        QJsonValue position = tempo_entry.toObject()["position"];
        QJsonValue bpm = tempo_entry.toObject()["bpm"];
        if (position.isDouble() && bpm.isDouble())
        {
            // The actual length of a tick is `one_minute / (bpm * sample_rate * TICK_RESOLUTION)`,
            // but we skip diving by TICK_RESOLUTION because otherwise the result would be smaller
            // than 1 ns. We divide by TICK_RESOLUTION after calling TicksToTime instead.
            //
            // We do have to divide by either sample_rate or TICK_RESOLUTION here, otherwise we'll
            // get 64-bit integer overflow inside TicksToTime.
            const std::chrono::nanoseconds one_minute = std::chrono::minutes(1);
            const std::chrono::nanoseconds time(static_cast<std::chrono::nanoseconds::rep>(
                one_minute.count() / (bpm.toDouble() * sample_rate)));
            tempo_map.AddTempoEntry(ToLongLong(position), time);
        }
    }
    if (tempo_map.empty())
        return nullptr;

    std::unique_ptr<ReadOnlySong> song = nullptr;

    const QJsonArray tracks = root["tracks"].toArray();
    for (const auto& track : tracks)
    {
        if (!song)
            song = std::make_unique<ReadOnlySong>();

        song->m_lines.emplace_back(std::make_unique<ReadOnlyLine>());

        const QJsonArray notes = track.toObject()["mainGroup"].toObject()["notes"].toArray();
        auto& syllables = song->m_lines.back()->m_syllables;
        for (const auto& note : notes)
        {
            QString lyric(note["lyrics"].toString());

            if (!syllables.empty())
            {
                if (!lyric.isEmpty() && lyric.front() != QChar('+') && lyric.front() != QChar('-'))
                    syllables[syllables.size() - 1]->m_text.append(QChar(' '));
            }

            const qint64 onset = ToLongLong(note["onset"]);
            const qint64 duration = ToLongLong(note["duration"]);

            const auto ticks_to_time = [&tempo_map, sample_rate](qint64 ticks) {
                return Centiseconds(tempo_map.TicksToTime(ticks).count() / TIME_CONVERSION_DIVISOR);
            };

            const Centiseconds start = ticks_to_time(onset);
            const Centiseconds end = ticks_to_time(onset + duration);

            syllables.emplace_back(std::make_unique<ReadOnlySyllable>(lyric, start, end));
        }
    }

    return song;
}

}
