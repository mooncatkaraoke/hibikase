// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <cinttypes>
#include <chrono>
#include <exception>
#include <memory>
#include <ratio>

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>

namespace
{
    class NotSupported final : public std::exception
    {
        virtual const char* what() const noexcept
        {
            return "Not supported";
        }
    } not_supported;
}

namespace KaraokeData
{
class ReadOnlyLine;

typedef std::chrono::duration<int32_t, std::centi> Centiseconds;

static constexpr Centiseconds PLACEHOLDER_TIME = Centiseconds(-1);

// Largest and smallest times safely representable in Soramimi [mm:ss:cc] format
static constexpr Centiseconds MAXIMUM_TIME = Centiseconds(100 * 60 * 100 - 1);
static constexpr Centiseconds MINIMUM_TIME = Centiseconds(0);

class Syllable : public QObject
{
    Q_OBJECT

public:
    virtual ~Syllable() = default;

    virtual QString GetText() const = 0;
    virtual void SetText(const QString& text) = 0;
    virtual Centiseconds GetStart() const = 0;
    virtual void SetStart(Centiseconds time) = 0;
    virtual Centiseconds GetEnd() const = 0;
    virtual void SetEnd(Centiseconds time) = 0;
};

class Line : public QObject
{
    Q_OBJECT

public:
    virtual ~Line() = default;

    virtual QVector<Syllable*> GetSyllables() = 0;
    virtual QVector<const Syllable*> GetSyllables() const = 0;
    virtual Centiseconds GetStart() const = 0;
    virtual Centiseconds GetEnd() const = 0;
    virtual QString GetPrefix() const = 0;
    virtual void SetPrefix(const QString& text) = 0;
    virtual QString GetText() const;

    virtual int PositionFromRaw(int) const { throw not_supported; }
    virtual int PositionToRaw(int) const { throw not_supported; }

    void Split(int split_position, ReadOnlyLine* first_out, ReadOnlyLine* second_out,
               bool* syllable_boundary_at_split_point_out) const;
    void Join(const Line& other, bool syllable_boundary_at_split_point, ReadOnlyLine* out) const;

protected slots:
    virtual void BuildText();

protected:
    QString m_text;
};

struct SongPosition final
{
    int line;
    int position_in_line;

    bool operator==(SongPosition other)
    {
        return line == other.line && position_in_line == other.position_in_line;
    }
    bool operator!=(SongPosition other) { return !operator==(other); }
};

class Song : public QObject
{
    Q_OBJECT

public:
    virtual ~Song() = default;

    virtual bool IsValid() const = 0;
    virtual bool IsEditable() const = 0;
    virtual QString GetRaw(int start_line, int end_line) const = 0;
    virtual QString GetRaw() const = 0;
    virtual QByteArray GetRawBytes() const = 0;
    virtual QVector<Line*> GetLines() = 0;
    virtual QVector<const Line*> GetLines() const = 0;
    virtual void ReplaceLine(int line_index, const Line* replace_with) = 0;
    virtual void ReplaceLines(int start_line, int lines_to_remove,
                              const QVector<const Line*>& replace_with) = 0;
    virtual void RemoveAllLines() = 0;
    virtual QString GetText(int start_line, int end_line) const;
    virtual QString GetText() const;

    // For lines which are fully covered by the supplied range (the start and end arguments),
    // a direct pointer to the line is added to the result vector. For lines which are partially
    // covered by the supplied range, the covered and non-covered parts of the line are stored
    // into one out parameter each, and a pointer to the covered part is added to the result vector.
    virtual QVector<const Line*> GetLines(SongPosition start, SongPosition end,
                                          ReadOnlyLine* first_line_first_half_out,
                                          ReadOnlyLine* first_line_last_half_out,
                                          ReadOnlyLine* last_line_first_half_out,
                                          ReadOnlyLine* last_line_last_half_out,
                                          bool* syllable_boundary_at_start_out,
                                          bool* syllable_boundary_at_end_out) const;

    // For the result to make sense, first_line_first_half, last_line_last_half,
    // syllable_boundary_at_start and syllable_boundary_at_end should be set as if
    // obtained from a call to GetLines that used the same start and end arguments.
    virtual void ReplaceLines(SongPosition start, SongPosition end,
                              const QVector<const Line*>& replace_with,
                              const Line* first_line_first_half, const Line* last_line_last_half,
                              bool syllable_boundary_at_start, bool syllable_boundary_at_end);

    virtual bool SupportsPositionConversion() const { return false; }
    virtual SongPosition PositionFromRaw(int raw_position) const
    {
        (void)raw_position;
        throw not_supported;
    }
    virtual int PositionToRaw(SongPosition position) const
    {
        (void)position;
        throw not_supported;
    }
    virtual void UpdateRawText(int position, int chars_to_remove, QStringRef replace_with)
    {
        (void)position;
        (void)chars_to_remove;
        (void)replace_with;
        throw not_supported;
    }

signals:
    void LinesChanged(int line_position, int lines_removed, int lines_added,
                      int raw_position, int raw_chars_removed, int raw_chars_added);
};

std::unique_ptr<Song> Load(const QByteArray& data);

}
