// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <chrono>
#include <ratio>
#include <map>

#include <QtGlobal>

namespace KaraokeData
{

using DoubleCentiseconds = std::chrono::duration<double, std::centi>;

class TempoMap
{
public:
    TempoMap();

    void AddTempoEntry(qint64 start_ticks, DoubleCentiseconds tick_duration);

    DoubleCentiseconds TicksToTime(qint64 ticks) const;

    bool empty() const { return m_tempo_map.empty(); }

private:
    struct TempoEntry
    {
        DoubleCentiseconds tick_duration = DoubleCentiseconds::zero();
        DoubleCentiseconds start_time = DoubleCentiseconds::zero();
        qint64 start_ticks = 0;
    };

    // The key is the tick a given tempo entry ends at
    std::map<qint64, TempoEntry> m_tempo_map;
};

} // namespace KaraokeData
