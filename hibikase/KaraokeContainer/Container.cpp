// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include <memory>

#include <QString>

#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"

namespace KaraokeContainer
{

std::unique_ptr<Container> Load(const QString& path)
{
    return std::make_unique<PlainContainer>(path);
}

}
