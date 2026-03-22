// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include "KaraokeData/SvpParser.h"

#include <chrono>
#include <memory>
#include <ratio>
#include <unordered_map>

#include <QByteArray>
#include <QChar>
#include <QDebug>
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

static long long ToLongLong(const QJsonValue& value)
{
    return value.toDouble();
}

static void ParseNotes(const QJsonArray& notes, const TempoMap& tempo_map, qint64 tick_offset,
                       ReadOnlySong* song)
{
    song->m_lines.emplace_back(std::make_unique<ReadOnlyLine>());

    auto& syllables = song->m_lines.back()->m_syllables;
    for (const auto& note : notes)
    {
        const QString lyric(note["lyrics"].toString());
        const qint64 onset = ToLongLong(note["onset"]);
        const qint64 duration = ToLongLong(note["duration"]);

        const Centiseconds start = std::chrono::duration_cast<Centiseconds>(
            tempo_map.TicksToTime(tick_offset + onset));
        const Centiseconds end = std::chrono::duration_cast<Centiseconds>(
            tempo_map.TicksToTime(tick_offset + onset + duration));

        syllables.emplace_back(std::make_unique<ReadOnlySyllable>(lyric, start, end));
    }
}

std::unique_ptr<Song> ParseSvp(const QByteArray& data)
{
    const QJsonDocument json = QJsonDocument::fromJson(data);
    if (json.isNull())
        return nullptr;

    const QJsonObject root = json.object();

    const QJsonValue sample_rate_value = root["renderConfig"]["sampleRate"];
    if (!sample_rate_value.isDouble())
        return nullptr;
    const qint64 sample_rate = ToLongLong(sample_rate_value);
    if (sample_rate == 0)
        return nullptr;

    TempoMap tempo_map;
    const QJsonArray tempo = root["time"]["tempo"].toArray();
    for (const auto& tempo_entry : tempo)
    {
        const QJsonValue position = tempo_entry["position"];
        const QJsonValue bpm = tempo_entry["bpm"];
        if (position.isDouble() && bpm.isDouble())
        {
            const DoubleCentiseconds one_minute = std::chrono::minutes(1);
            const DoubleCentiseconds time =
                    one_minute / (bpm.toDouble() * sample_rate * TICK_RESOLUTION);
            tempo_map.AddTempoEntry(ToLongLong(position), time);
        }
    }
    if (tempo_map.empty())
        return nullptr;

    std::unordered_map<QString, QJsonArray> library_notes;
    const QJsonArray library = root["library"].toArray();
    for (const auto& library_entry : library)
    {
        const QJsonValue uuid = library_entry["uuid"];
        if (uuid.isString())
            library_notes[uuid.toString()] = library_entry["notes"].toArray();
    }

    std::unique_ptr<ReadOnlySong> song = nullptr;
    const QJsonArray tracks = root["tracks"].toArray();
    for (const auto& track : tracks)
    {
        if (!song)
            song = std::make_unique<ReadOnlySong>();

        const qint64 blick_absolute_begin = ToLongLong(track["mainRef"]["blickAbsoluteBegin"]);
        const QJsonArray notes = track["mainGroup"]["notes"].toArray();
        ParseNotes(notes, tempo_map, blick_absolute_begin, song.get());

        const QJsonArray groups = track["groups"].toArray();
        for (const auto& group : groups)
        {
            const QJsonValue group_id = group["groupID"];
            const qint64 blick_absolute_begin = ToLongLong(group["blickAbsoluteBegin"]);
            if (group_id.isString())
            {
                const auto it = library_notes.find(group_id.toString());
                if (it == library_notes.end())
                    qWarning() << "Group " + group_id.toString() + " not found in library";
                else
                    ParseNotes(it->second, tempo_map, blick_absolute_begin, song.get());
            }
        }
    }

    return song;
}

}
