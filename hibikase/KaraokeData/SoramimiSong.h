// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

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

class SoramimiSong;

class SoramimiSyllable final : public Syllable
{
    Q_OBJECT

    friend class SoramimiLine;

public:
    SoramimiSyllable(const QString& text, Centiseconds start,
                            Centiseconds end);

    QString GetText() const override { return m_text; }
    void SetText(const QString& text) override;
    Centiseconds GetStart() const override { return m_start; }
    void SetStart(Centiseconds time) override;
    Centiseconds GetEnd() const override { return m_end; }
    void SetEnd(Centiseconds time) override;

signals:
    void Changed();

private:
    QString m_text;
    Centiseconds m_start;
    Centiseconds m_end;
};

class SoramimiLine final : public Line
{
    Q_OBJECT

public:
    SoramimiLine(const QString& content);
    SoramimiLine(const QVector<const Syllable*>& syllables, QString prefix = QString());

    QVector<Syllable*> GetSyllables() override;
    QVector<const Syllable*> GetSyllables() const override;
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }
    QString GetPrefix() const override { return m_prefix; }
    void SetPrefix(const QString& text) override;
    QString GetRaw() const { return m_raw_content; }
    void SetRaw(QString raw);

    int PositionFromRaw(int raw_position) const override;
    int PositionToRaw(int position) const override;

signals:
    void Changed(int old_raw_length, int new_raw_length);

private:
    void Serialize();
    void Serialize(const QVector<const Syllable*>& syllables);
    void Deserialize();
    void CalculateStartAndEnd();
    void AddSyllable(int start, int end, Centiseconds start_time, Centiseconds end_time);

    static QString SerializeTime(Centiseconds time);
    static QString SerializeNumber(int number, int digits);

    QString m_raw_content;
    std::vector<int> m_raw_syllable_positions;

    std::vector<std::unique_ptr<SoramimiSyllable>> m_syllables;
    Centiseconds m_start;
    Centiseconds m_end;
    QString m_prefix;
};

class SoramimiSong final : public Song
{
    Q_OBJECT

    friend class SoramimiLine;

public:
    SoramimiSong(const QByteArray& data);
    SoramimiSong(const QVector<const Line*>& lines);

    bool IsValid() const override { return true; }
    bool IsEditable() const override { return true; }
    QString GetRaw(int start_line, int end_line) const override;
    QString GetRaw() const override;
    QByteArray GetRawBytes() const override;
    QVector<Line*> GetLines() override;
    QVector<const Line*> GetLines() const override;
    void ReplaceLine(int start_line, const Line* replace_with) override;
    void ReplaceLines(int start_line, int lines_to_remove,
                      const QVector<const Line*>& replace_with) override;
    void RemoveAllLines() override;

    bool SupportsPositionConversion() const override;
    SongPosition PositionFromRaw(int raw_position) const override;
    int PositionToRaw(SongPosition position) const override;
    void UpdateRawText(int position, int chars_to_remove, QStringRef replace_with) override;

private:
    SongPosition RawPositionFromRaw(int raw_position) const;
    int LineNumberToRaw(int line) const;
    std::unique_ptr<SoramimiLine> SetUpLine(std::unique_ptr<SoramimiLine> line);
    void EmitLineChanged(const SoramimiLine* line, int old_raw_length, int new_raw_length);

    std::vector<std::unique_ptr<SoramimiLine>> m_lines;
    bool m_updates_disabled = false;
};

}
