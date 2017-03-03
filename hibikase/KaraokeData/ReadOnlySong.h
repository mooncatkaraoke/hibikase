// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <exception>
#include <vector>

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
    ReadOnlySyllable(const std::string& text, Centiseconds start, Centiseconds end)
        : m_text(text), m_start(start), m_end(end)
    {
    }

    virtual std::string GetText() const override { return m_text; }
    virtual Centiseconds GetStart() const override { return m_start; }
    virtual Centiseconds GetEnd() const override { return m_end; }

    std::string m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class ReadOnlyLine final : public Line
{
public:
    virtual std::vector<Syllable*> GetSyllables() override
    {
        std::vector<Syllable*> result;
        result.reserve(m_syllables.size());
        for (ReadOnlySyllable& syllable : m_syllables)
            result.push_back(&syllable);
        return result;
    }
    virtual Centiseconds GetStart() const override { throw not_editable; }
    virtual Centiseconds GetEnd() const override { throw not_editable; }
    const std::string& GetRaw() const { throw not_editable; }

    std::vector<ReadOnlySyllable> m_syllables;
};

class ReadOnlySong final : public Song
{
public:
    virtual bool IsValid() const override { return m_valid; }
    virtual bool IsEditable() const override { return false; }
    virtual std::string GetRaw() const override { throw not_editable; }
    virtual std::vector<Line*> GetLines() override
    {
        std::vector<Line*> result;
        result.reserve(m_lines.size());
        for (ReadOnlyLine& line : m_lines)
            result.push_back(&line);
        return result;
    }
    virtual void AddLine(const std::vector<Syllable*>&) override { throw not_editable; }
    virtual void RemoveAllLines() override { throw not_editable; }

    bool m_valid = false;
    std::vector<ReadOnlyLine> m_lines;
};

}
