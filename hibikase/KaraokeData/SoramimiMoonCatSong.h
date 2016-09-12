// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <chrono>
#include <string>
#include <ratio>
#include <vector>

#include "KaraokeData/Song.h"

namespace KaraokeData
{

// These typedefs are both for having shorter code
// and for using a smaller data type for storing seconds
// compared to std::chrono::seconds
typedef std::chrono::duration<int32_t> Seconds;
typedef std::chrono::duration<int32_t, std::ratio<60, 1>> Minutes;

class SoramimiMoonCatSyllable final : public Syllable
{
    friend class SoramimiMoonCatLine;

public:
    SoramimiMoonCatSyllable(const std::string& text, Centiseconds start,
                            Centiseconds end);

    virtual std::string GetText() const override { return m_text; }
    virtual Centiseconds GetStart() const override { return m_start; }
    virtual Centiseconds GetEnd() const override { return m_end; }

private:
    std::string m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class SoramimiMoonCatLine final : public Line
{
public:
    SoramimiMoonCatLine(const std::string& content);
    SoramimiMoonCatLine(const std::vector<Syllable*>& syllables);

    virtual std::vector<Syllable*> GetSyllables() override;
    virtual Centiseconds GetStart() const override { return m_start; }
    virtual Centiseconds GetEnd() const override { return m_end; }
    const std::string& GetRaw() const { return m_raw_content; }

private:
    void Serialize(const std::vector<Syllable*>& syllables);
    void Deserialize();

    static std::string SerializeTime(Centiseconds time);
    static std::string SerializeNumber(int number, size_t digits);

    // Returns -1 if there aren't as many digits as requested.
    // Does not check that the position is in range.
    static int DeserializeNumber(const std::string& string, size_t position, size_t digits);

    std::string m_raw_content;

    std::vector<SoramimiMoonCatSyllable> m_syllables;
    Centiseconds m_start;
    Centiseconds m_end;
};

class SoramimiMoonCatSong final : public Song
{
public:
    SoramimiMoonCatSong(const std::vector<char>& data);
    // TODO: Use this constructor when converting, and remove the AddLine function
    SoramimiMoonCatSong(const std::vector<Line*>& lines);

    virtual bool IsValid() const override { return true; }
    virtual bool IsEditable() const override { return true; }
    virtual std::string GetRaw() const override;
    virtual std::vector<Line*> GetLines() override;
    virtual void AddLine(const std::vector<Syllable*>& syllables) override;

private:
    std::vector<SoramimiMoonCatLine> m_lines;
};

}
