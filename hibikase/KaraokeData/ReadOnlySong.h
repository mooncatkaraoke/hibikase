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

#include <exception>
#include <memory>
#include <vector>

#include <QString>

#include "KaraokeData/Song.h"

namespace
{
    class NotEditable final : public std::exception
    {
        virtual const char* what() const throw()
        {
            return "Not editable";
        }
    } not_editable;
}

namespace KaraokeData
{

class ReadOnlySyllable final : public Syllable
{
    Q_OBJECT

public:
    // Note: This default constructor won't initialize class members properly
    ReadOnlySyllable() {}
    ReadOnlySyllable(const QString& text, Centiseconds start, Centiseconds end)
        : m_text(text), m_start(start), m_end(end)
    {
    }

    QString GetText() const override { return m_text; }
    void SetText(const QString&) override { throw not_editable; }
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }

    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class ReadOnlyLine final : public Line
{
    Q_OBJECT

public:
    virtual QVector<Syllable*> GetSyllables() override
    {
        QVector<Syllable*> result;
        result.reserve(m_syllables.size());
        for (std::unique_ptr<ReadOnlySyllable>& syllable : m_syllables)
            result.push_back(syllable.get());
        return result;
    }
    Centiseconds GetStart() const override { throw not_editable; }
    Centiseconds GetEnd() const override { throw not_editable; }
    QString GetPrefix() const override { return m_prefix; }
    void SetPrefix(const QString&) override { throw not_editable; }
    QString GetText() const override
    {
        const_cast<ReadOnlyLine*>(this)->BuildText();
        return Line::GetText();
    }
    void SetSyllableSplitPoints(QVector<int>) override { throw not_editable; }

    std::vector<std::unique_ptr<ReadOnlySyllable>> m_syllables;
    QString m_prefix;
};

class ReadOnlySong final : public Song
{
    Q_OBJECT

public:
    bool IsValid() const override { return m_valid; }
    bool IsEditable() const override { return false; }
    QString GetRaw() const override { throw not_editable; }
    QByteArray GetRawBytes() const override { throw not_editable; }
    QVector<Line*> GetLines() override
    {
        QVector<Line*> result;
        result.reserve(m_lines.size());
        for (std::unique_ptr<ReadOnlyLine>& line : m_lines)
            result.push_back(line.get());
        return result;
    }
    void AddLine(const QVector<Syllable*>&, QString) override { throw not_editable; }
    void RemoveAllLines() override { throw not_editable; }

    bool m_valid = false;
    std::vector<std::unique_ptr<ReadOnlyLine>> m_lines;
};

}
