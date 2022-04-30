// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <memory>

#include <QByteArray>
#include <QIODevice>
#include <QString>

#include "KaraokeContainer/Container.h"

namespace KaraokeContainer
{

class PlainContainer final : public Container
{
public:
    PlainContainer(const QString& path);

    std::unique_ptr<QIODevice> ReadAudioFile() const override;
    QByteArray ReadLyricsFile() const override;
    bool SaveLyricsFile(const QByteArray& content) const override;

private:
    QString m_path;
};

}
