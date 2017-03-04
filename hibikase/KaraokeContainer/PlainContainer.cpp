// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QString>

#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"

namespace KaraokeContainer
{

PlainContainer::PlainContainer(const QString& path)
    : m_path(path)
{
}

QByteArray PlainContainer::ReadLyricsFile()
{
    QFile file(m_path);
    // TODO: Should this be an exception?
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

void PlainContainer::SaveLyricsFile(const QString& path, const QByteArray& content)
{
    QFile file(path);
    // TODO: Should this be an exception?
    if (!file.open(QIODevice::WriteOnly))
        return;
    // TODO: Check if this fails?
    file.write(content.constData(), content.size());
}

}
