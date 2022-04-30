// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <memory>

#include <QByteArray>

#include "KaraokeData/Song.h"

namespace KaraokeData
{

std::unique_ptr<Song> ParseVsqx(const QByteArray& data);

}
