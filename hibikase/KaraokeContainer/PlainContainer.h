// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// As an additional permission for this file only, you can (at your
// option) instead use this file under the terms of CC0.
// <http://creativecommons.org/publicdomain/zero/1.0/>

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <QByteArray>
#include <QString>

#include "KaraokeContainer/Container.h"

namespace KaraokeContainer
{

class PlainContainer final : public Container
{
public:
    PlainContainer(const QString& path);

    QByteArray ReadLyricsFile() const override;
    void SaveLyricsFile(const QByteArray& content) const override;

private:
    QString m_path;
};

}
