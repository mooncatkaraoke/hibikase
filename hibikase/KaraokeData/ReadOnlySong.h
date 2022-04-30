// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <exception>
#include <memory>
#include <vector>
#include <utility>

#include <QString>

#include "KaraokeData/Song.h"

namespace
{
class NotEditable final : public std::exception
{
    virtual const char* what() const noexcept
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
    ReadOnlySyllable(QString text, Centiseconds start, Centiseconds end)
        : m_text(std::move(text)), m_start(start), m_end(end)
    {
    }
    ReadOnlySyllable(const Syllable& syllable)
        : m_text(syllable.GetText()), m_start(syllable.GetStart()), m_end(syllable.GetEnd())
    {
    }

    QString GetText() const override { return m_text; }
    void SetText(const QString&) override { throw not_editable; }
    Centiseconds GetStart() const override { return m_start; }
    void SetStart(Centiseconds) override { throw not_editable; }
    Centiseconds GetEnd() const override { return m_end; }
    void SetEnd(Centiseconds) override { throw not_editable; }

    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class ReadOnlyLine final : public Line
{
    Q_OBJECT

public:
    ReadOnlyLine() {}
    ReadOnlyLine(std::vector<std::unique_ptr<ReadOnlySyllable>> syllables, QString prefix)
        : m_syllables(std::move(syllables)), m_prefix(std::move(prefix))
    {
    }

    virtual QVector<Syllable*> GetSyllables() override
    {
        QVector<Syllable*> result;
        result.reserve(m_syllables.size());
        for (std::unique_ptr<ReadOnlySyllable>& syllable : m_syllables)
            result.push_back(syllable.get());
        return result;
    }
    virtual QVector<const Syllable*> GetSyllables() const override
    {
        QVector<const Syllable*> result;
        result.reserve(m_syllables.size());
        for (const std::unique_ptr<ReadOnlySyllable>& syllable : m_syllables)
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

    std::vector<std::unique_ptr<ReadOnlySyllable>> m_syllables;
    QString m_prefix;
};

class ReadOnlySong final : public Song
{
    Q_OBJECT

public:
    bool IsValid() const override { return m_valid; }
    bool IsEditable() const override { return false; }
    QString GetRaw(int, int) const override { throw not_editable; }
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
    QVector<const Line*> GetLines() const override
    {
        QVector<const Line*> result;
        result.reserve(m_lines.size());
        for (const std::unique_ptr<ReadOnlyLine>& line : m_lines)
            result.push_back(line.get());
        return result;
    }
    void ReplaceLine(int, const Line*) override { throw not_editable; }
    void ReplaceLines(int, int, const QVector<const Line*>&) override { throw not_editable; }
    void RemoveAllLines() override { throw not_editable; }

    bool m_valid = false;
    std::vector<std::unique_ptr<ReadOnlyLine>> m_lines;
};

}
