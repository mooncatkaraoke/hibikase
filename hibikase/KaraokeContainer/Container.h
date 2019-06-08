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

#include <memory>

#include <QByteArray>
#include <QIODevice>
#include <QString>

namespace KaraokeContainer
{

class Container
{
public:
    virtual ~Container() = default;

    // Returns a read-only QIODevice that already is open, or nullptr
    virtual std::unique_ptr<QIODevice> ReadAudioFile() const = 0;

    virtual QByteArray ReadLyricsFile() const = 0;
    virtual void SaveLyricsFile(const QByteArray& content) const = 0;
};

std::unique_ptr<Container> Load(const QString& path);

}
