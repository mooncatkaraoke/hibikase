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
