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
    Centiseconds GetEnd() const override { return m_end; }

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
    SoramimiLine(const QVector<Syllable*>& syllables, QString prefix = QString());

    QVector<Syllable*> GetSyllables() override;
    Centiseconds GetStart() const override { return m_start; }
    Centiseconds GetEnd() const override { return m_end; }
    QString GetPrefix() const override { return m_prefix; }
    void SetPrefix(const QString& text) override;
    QString GetRaw() const { return m_raw_content; }
    // All split points must be unique and in ascending order
    void SetSyllableSplitPoints(QVector<int> split_points) override;

    int PositionFromRaw(int raw_position) const override;
    int PositionToRaw(int position) const override;

private:
    void Serialize();
    void Serialize(const QVector<Syllable*>& syllables);
    void Deserialize();
    void AddSyllable(size_t start, size_t end, Centiseconds start_time, Centiseconds end_time);

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

public:
    SoramimiSong(const QByteArray& data);
    // TODO: Use this constructor when converting, and remove the AddLine function
    SoramimiSong(const QVector<Line*>& lines);

    bool IsValid() const override { return true; }
    bool IsEditable() const override { return true; }
    QString GetRaw() const override;
    QByteArray GetRawBytes() const override;
    QVector<Line*> GetLines() override;
    void AddLine(const QVector<Syllable*>& syllables, QString prefix) override;
    void RemoveAllLines() override;

    bool SupportsPositionConversion() const override;
    SongPosition PositionFromRaw(int raw_position) const override;
    int PositionToRaw(SongPosition position) const override;

private:
    std::vector<std::unique_ptr<SoramimiLine>> m_lines;
};

}
