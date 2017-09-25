// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <chrono>
#include <memory>
#include <ratio>
#include <vector>

#include <QByteArray>
#include <QObject>
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
    Q_OBJECT

    friend class SoramimiMoonCatLine;

public:
    SoramimiMoonCatSyllable(const QString& text, Centiseconds start,
                            Centiseconds end);

    QString GetText() const override { return m_text; }
    void SetText(const QString& text) override;
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }

signals:
    void Changed();

private:
    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class SoramimiMoonCatLine final : public Line
{
    Q_OBJECT

public:
    SoramimiMoonCatLine(const QString& content);
    SoramimiMoonCatLine(const QVector<Syllable*>& syllables,
                        QString prefix = QString(), QString suffix = QString());

    QVector<Syllable*> GetSyllables() override;
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }
    QString GetPrefix() const override { return m_prefix; }
    void SetPrefix(const QString& text) override;
    QString GetSuffix() const override { return m_suffix; }
    void SetSuffix(const QString& text) override;
    QString GetRaw() const { return m_raw_content; }
    // All split points must be unique and in ascending order
    void SetSyllableSplitPoints(QVector<int> split_points) override;

private:
    void Serialize();
    void Serialize(const QVector<Syllable*>& syllables);
    void Deserialize();

    static QString SerializeTime(Centiseconds time);
    static QString SerializeNumber(int number, int digits);

    QString m_raw_content;

    std::vector<std::unique_ptr<SoramimiMoonCatSyllable>> m_syllables;
    Centiseconds m_start;
    Centiseconds m_end;
    QString m_prefix;
    QString m_suffix;
};

class SoramimiMoonCatSong final : public Song
{
    Q_OBJECT

public:
    SoramimiMoonCatSong(const QByteArray& data);
    // TODO: Use this constructor when converting, and remove the AddLine function
    SoramimiMoonCatSong(const QVector<Line*>& lines);

    bool IsValid() const override { return true; }
    bool IsEditable() const override { return true; }
    QString GetRaw() const override;
    QByteArray GetRawBytes() const override;
    QVector<Line*> GetLines() override;
    void AddLine(const QVector<Syllable*>& syllables, QString prefix, QString suffix) override;
    void RemoveAllLines() override;

private:
    std::vector<std::unique_ptr<SoramimiMoonCatLine>> m_lines;
};

}
