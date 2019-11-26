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

#include <vector>

#include <QIODevice>
#include <QObject>

class CircularBuffer : public QIODevice
{
    Q_OBJECT

public:
    CircularBuffer(size_t buffer_size, QObject* parent = nullptr);

    bool isSequential() const override { return true; }

    size_t bufferSize() const { return m_buffer.size(); }
    size_t unreadDataSize() const { return m_unread_bytes; }
    void clearData();
    void clearData(size_t buffer_size);

signals:
    void bytesRead(qint64 bytes);

protected:
    // Reads up to unreadDataSize() bytes of data
    qint64 readData(char* data, qint64 maxlen) override;
    // Writes up to bufferSize() bytes of data
    qint64 writeData(const char* data, qint64 len) override;

private:
    std::vector<char> m_buffer;
    size_t m_read_offset;
    size_t m_unread_bytes;
};
