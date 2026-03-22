// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include "KaraokeData/Song.h"

#include <QString>
#include <QVector>

namespace TextTransform
{

std::unique_ptr<KaraokeData::Line> AddSpacesBetweenWords(
    const QVector<const KaraokeData::Syllable*>& syllables, QString prefix);

}
