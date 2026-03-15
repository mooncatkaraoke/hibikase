// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <chrono>
#include <map>

namespace KaraokeData
{

class TempoMap
{
public:
    TempoMap();

    void AddTempoEntry(int start_ticks, std::chrono::nanoseconds tick_duration);

    std::chrono::nanoseconds TicksToTime(int ticks) const;

    bool empty() const { return m_tempo_map.empty(); }

private:
    struct TempoEntry
    {
        std::chrono::nanoseconds tick_duration = std::chrono::nanoseconds::zero();
        std::chrono::nanoseconds start_time = std::chrono::nanoseconds::zero();
        int start_ticks = 0;
    };

    static constexpr int MAX_TICK = std::numeric_limits<int>().max();

    // The key is the tick a given tempo entry ends at
    std::map<int, TempoEntry> m_tempo_map;
};

} // namespace KaraokeData
