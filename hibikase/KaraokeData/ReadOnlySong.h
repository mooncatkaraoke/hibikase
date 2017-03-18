// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <exception>

#include <QString>
#include <QVector>

#include "KaraokeData/Song.h"

namespace KaraokeData
{

class NotEditable final : public std::exception
{
    virtual const char* what() const throw()
    {
        return "Not editable";
    }
} not_editable;

class ReadOnlySyllable final : public Syllable
{
public:
    // Note: This default constructor won't initialize class members properly
    ReadOnlySyllable() {}
    ReadOnlySyllable(const QString& text, Centiseconds start, Centiseconds end)
        : m_text(text), m_start(start), m_end(end)
    {
    }

    QString GetText() const override { return m_text; }
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }

    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class ReadOnlyLine final : public Line
{
public:
    virtual QVector<Syllable*> GetSyllables() override
    {
        QVector<Syllable*> result;
        result.reserve(m_syllables.size());
        for (ReadOnlySyllable& syllable : m_syllables)
            result.push_back(&syllable);
        return result;
    }
    Centiseconds GetStart() const override { throw not_editable; }
    Centiseconds GetEnd() const override { throw not_editable; }
    QString GetPrefix() const override { return m_prefix; }
    QString GetSuffix() const override { return m_suffix; }

    QVector<ReadOnlySyllable> m_syllables;
    QString m_prefix;
    QString m_suffix;
};

class ReadOnlySong final : public Song
{
public:
    bool IsValid() const override { return m_valid; }
    bool IsEditable() const override { return false; }
    QString GetRaw() const override { throw not_editable; }
    QByteArray GetRawBytes() const override { throw not_editable; }
    QVector<Line*> GetLines() override
    {
        QVector<Line*> result;
        result.reserve(m_lines.size());
        for (ReadOnlyLine& line : m_lines)
            result.push_back(&line);
        return result;
    }
    void AddLine(const QVector<Syllable*>&) override { throw not_editable; }
    void RemoveAllLines() override { throw not_editable; }

    bool m_valid = false;
    QVector<ReadOnlyLine> m_lines;
};

}
