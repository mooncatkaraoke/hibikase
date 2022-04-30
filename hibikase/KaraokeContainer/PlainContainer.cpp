// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include <QStringList>

#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"

namespace KaraokeContainer
{

PlainContainer::PlainContainer(const QString& path)
    : m_path(path)
{
}

std::unique_ptr<QIODevice> PlainContainer::ReadAudioFile() const
{
    const QDir dir = QFileInfo(m_path).dir();
    QStringList filters;
    filters << "*.mp3" << "*.flac";
    for (const QFileInfo& file_info : dir.entryInfoList(filters))
    {
        std::unique_ptr<QIODevice> file = std::make_unique<QFile>(file_info.absoluteFilePath());
        if (file->open(QIODevice::ReadOnly))
            return file;
    }
    return nullptr;
}

QByteArray PlainContainer::ReadLyricsFile() const
{
    QFile file(m_path);
    // TODO: Should this be an exception?
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

bool PlainContainer::SaveLyricsFile(const QByteArray& content) const
{
    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(content.constData(), content.size()) == content.size();
}

}
