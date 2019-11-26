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

#include "CircularBuffer.h"

#include <algorithm>
#include <cstring>

CircularBuffer::CircularBuffer(size_t buffer_size, QObject* parent)
    : QIODevice(parent)
{
    clearData(buffer_size);
}

qint64 CircularBuffer::readData(char* data, qint64 maxlen)
{
    size_t bytes_to_read = std::min<size_t>(m_unread_bytes, maxlen);
    size_t bytes_read = 0;
    while (bytes_to_read > 0)
    {
        const size_t n = std::min(m_buffer.size() - m_read_offset, bytes_to_read);
        std::memcpy(data + bytes_read, m_buffer.data() + m_read_offset, n);
        bytes_to_read -= n;
        bytes_read += n;
        m_unread_bytes -= n;
        m_read_offset += n;
        if (m_read_offset == m_buffer.size())
            m_read_offset = 0;
    }

    if (bytes_read > 0)
        emit bytesRead(bytes_read);

    return bytes_read;
}

qint64 CircularBuffer::writeData(const char* data, qint64 len)
{
    size_t write_offset = m_read_offset + m_unread_bytes;
    if (write_offset > m_buffer.size())
        write_offset -= m_buffer.size();

    size_t bytes_to_write = std::min<size_t>(m_buffer.size() - m_unread_bytes, len);
    size_t bytes_written = 0;
    while (bytes_to_write > 0)
    {
        const size_t n = std::min(m_buffer.size() - write_offset, bytes_to_write);
        std::memcpy(m_buffer.data() + write_offset, data + bytes_written, n);
        bytes_to_write -= n;
        bytes_written += n;
        m_unread_bytes += n;
        write_offset += n;
        if (write_offset == m_buffer.size())
            write_offset = 0;
    }

    if (bytes_written > 0)
    {
        emit bytesWritten(bytes_written);
        emit readyRead();
    }

    return bytes_written;
}

void CircularBuffer::clearData()
{
    m_read_offset = 0;
    m_unread_bytes = 0;
}

void CircularBuffer::clearData(size_t buffer_size)
{
    m_buffer.resize(buffer_size);
    clearData();
}
