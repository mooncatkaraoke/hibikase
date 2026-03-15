// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <chrono>
#include <map>

#include <QtGlobal>

namespace KaraokeData
{

class TempoMap
{
public:
    TempoMap();

    void AddTempoEntry(qint64 start_ticks, std::chrono::nanoseconds tick_duration);

    std::chrono::nanoseconds TicksToTime(qint64 ticks) const;

    bool empty() const { return m_tempo_map.empty(); }

private:
    struct TempoEntry
    {
        std::chrono::nanoseconds tick_duration = std::chrono::nanoseconds::zero();
        std::chrono::nanoseconds start_time = std::chrono::nanoseconds::zero();
        qint64 start_ticks = 0;
    };

    static constexpr qint64 MAX_TICK = std::numeric_limits<qint64>().max();

    // The key is the tick a given tempo entry ends at
    std::map<qint64, TempoEntry> m_tempo_map;
};

} // namespace KaraokeData
