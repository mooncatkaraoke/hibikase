// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <chrono>
#include <ratio>

#include <QByteArray>
#include <QList>
#include <QString>
#include <QVector>

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
    // Note: This default constructor won't initialize class members properly
    SoramimiMoonCatSyllable();
    SoramimiMoonCatSyllable(const QString& text, Centiseconds start,
                            Centiseconds end);

    QString GetText() const override { return m_text; }
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }

private:
    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class SoramimiMoonCatLine final : public Line
{
public:
    SoramimiMoonCatLine(const QString& content);
    SoramimiMoonCatLine(const QVector<Syllable*>& syllables);

    QVector<Syllable*> GetSyllables() override;
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }
    QString GetPrefix() const override { return m_prefix; }
    QString GetSuffix() const override { return m_suffix; }
    QString GetRaw() const { return m_raw_content; }

private:
    void Serialize(const QVector<Syllable*>& syllables);
    void Deserialize();

    static QString SerializeTime(Centiseconds time);
    static QString SerializeNumber(int number, int digits);

    QString m_raw_content;

    QVector<SoramimiMoonCatSyllable> m_syllables;
    Centiseconds m_start;
    Centiseconds m_end;
    QString m_prefix;
    QString m_suffix;
};

class SoramimiMoonCatSong final : public Song
{
public:
    SoramimiMoonCatSong(const QByteArray& data);
    // TODO: Use this constructor when converting, and remove the AddLine function
    SoramimiMoonCatSong(const QVector<Line*>& lines);

    bool IsValid() const override { return true; }
    bool IsEditable() const override { return true; }
    QString GetRaw() const override;
    QByteArray GetRawBytes() const override;
    QVector<Line*> GetLines() override;
    void AddLine(const QVector<Syllable*>& syllables) override;
    void RemoveAllLines() override;

private:
    QList<SoramimiMoonCatLine> m_lines;
};

}
