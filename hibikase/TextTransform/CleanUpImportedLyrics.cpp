// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include "TextTransform/CleanUpImportedLyrics.h"

#include <memory>

#include <QString>
#include <QVector>

#include "KaraokeData/Song.h"
#include "KaraokeData/ReadOnlySong.h"

namespace TextTransform
{

std::unique_ptr<KaraokeData::Line> AddSpacesBetweenWords(
    const QVector<const KaraokeData::Syllable*>& syllables, QString prefix)
{
    std::unique_ptr<KaraokeData::ReadOnlyLine> line = std::make_unique<KaraokeData::ReadOnlyLine>();
    line->m_syllables.reserve(syllables.size());
    line->m_prefix = std::move(prefix);

    for (auto* syllable : syllables)
    {
        if (!line->m_syllables.empty())
        {
            const QString text = syllable->GetText();
            if (!text.isEmpty() && text.front() != QChar('+') && text.front() != QChar('-'))
            {
                KaraokeData::ReadOnlySyllable& last_syllable =
                    *line->m_syllables[line->m_syllables.size() - 1];

                if (last_syllable.m_text.isEmpty() || last_syllable.m_text.back() != QChar(' '))
                    last_syllable.m_text.append(QChar(' '));
            }
        }

        line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(*syllable));
    }

    return line;
}

}
