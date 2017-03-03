// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <cinttypes>
#include <chrono>
#include <memory>
#include <ratio>
#include <string>
#include <vector>

namespace KaraokeData
{

typedef std::chrono::duration<int32_t, std::centi> Centiseconds;

class Syllable
{
public:
    virtual std::string GetText() const = 0;
    virtual Centiseconds GetStart() const = 0;
    virtual Centiseconds GetEnd() const = 0;
};

class Line
{
public:
    virtual std::vector<Syllable*> GetSyllables() = 0;
    virtual Centiseconds GetStart() const = 0;
    virtual Centiseconds GetEnd() const = 0;
};

class Song
{
public:
    virtual bool IsValid() const = 0;
    virtual bool IsEditable() const = 0;
    virtual std::string GetRaw() const = 0;
    virtual std::vector<Line*> GetLines() = 0;
    // TODO: Should be const std::vector<const Syllable*>&
    virtual void AddLine(const std::vector<Syllable*>& syllables) = 0;
    virtual void RemoveAllLines() = 0;
};

std::unique_ptr<Song> Load(const std::vector<char>& data);

}
