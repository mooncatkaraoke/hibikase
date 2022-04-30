// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

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
    virtual bool SaveLyricsFile(const QByteArray& content) const = 0;
};

std::unique_ptr<Container> Load(const QString& path);

}
