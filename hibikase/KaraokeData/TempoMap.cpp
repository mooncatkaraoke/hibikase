// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include "KaraokeData/TempoMap.h"

#include <chrono>
#include <limits>

#include <QtGlobal>

namespace KaraokeData
{

TempoMap::TempoMap() = default;

void TempoMap::AddTempoEntry(qint64 start_ticks, DoubleCentiseconds tick_duration)
{
    TempoEntry entry{
        .tick_duration = tick_duration,
        .start_time = TicksToTime(start_ticks),
        .start_ticks = start_ticks,
    };

    if (m_tempo_map.empty())
    {
        m_tempo_map.emplace(MAX_TICK, entry);
        return;
    }

    auto it = m_tempo_map.lower_bound(start_ticks);
    if (it->first == MAX_TICK)
    {
        // The new entry goes at the end. Update the key of the entry that previously was last.
        m_tempo_map.emplace_hint(it, start_ticks, it->second);
        it->second = entry;
    }
    else
    {
        // The new entry goes in the middle. Update the start_time of all entries that come after.
        it = m_tempo_map.emplace_hint(it, it->second.start_ticks, entry);
        for (; it != m_tempo_map.end(); ++it)
            it->second.start_time = TicksToTime(it->second.start_ticks);
    }
}

DoubleCentiseconds TempoMap::TicksToTime(qint64 ticks) const
{
    if (m_tempo_map.empty())
        return DoubleCentiseconds::zero();

    auto it = m_tempo_map.lower_bound(ticks);
    const TempoEntry& entry = it->second;
    return entry.start_time + (ticks - entry.start_ticks) * entry.tick_duration;
}

} // namespace KaraokeData
