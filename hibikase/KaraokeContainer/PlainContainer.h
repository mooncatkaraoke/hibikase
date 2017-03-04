// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

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

    QByteArray ReadLyricsFile() override;

    static void SaveLyricsFile(const QString& path, const QByteArray& content);

private:
    QString m_path;
};

}
