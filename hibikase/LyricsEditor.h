// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

#include <QEvent>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPoint>
#include <QWidget>

#include "KaraokeData/Song.h"

#include "LineTimingDecorations.h"

class TimingEventFilter : public QObject
{
    Q_OBJECT

public:
    TimingEventFilter(QObject* parent = nullptr);

signals:
    void SetSyllableStart();
    void SetSyllableEnd();
    void GoToPreviousSyllable();
    void GoToNextSyllable();
    void GoToPreviousLine();
    void GoToNextLine();
    void GoToPosition(QPoint pos);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

class TextEventFilter : public QObject
{
    Q_OBJECT

public:
    TextEventFilter(QObject* parent = nullptr);

signals:
    void ToggleSyllable();

private:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

class LyricsEditor : public QWidget
{
    Q_OBJECT
public:

    enum class Mode
    {
        Timing,
        Text,
        Raw
    };

    struct SyllablePosition
    {
        int line;
        int syllable;

        bool IsValid() const { return line >= 0 && syllable >= 0; }
    };

    explicit LyricsEditor(QWidget* parent = nullptr);

    void SetMode(Mode mode);

    void AddActionsToMenu(QMenu* menu);

signals:
    void Modified();

public slots:
    void ReloadSong(KaraokeData::Song* song);
    void UpdateTime(std::chrono::milliseconds time);
    void UpdateSpeed(double speed);

private slots:
    void OnLinesChanged(int line_position, int lines_removed, int lines_added,
                        int raw_position, int raw_chars_removed, int raw_chars_added);
    void OnRawContentsChange(int position, int chars_removed, int chars_added);
    void OnCursorPositionChanged();

    void GoToPosition(QPoint pos);

    void AddSyllabificationLanguages(QMenu* menu);
    void Syllabify(const QString& language_code);
    void RomanizeHangul();
    void ShiftTimings();

    void SetSyllableStart();
    void SetSyllableEnd();
    void ToggleSyllable();

private:
    void AddLyricsActionsToMenu(QMenu* menu, QPlainTextEdit* text_edit);
    void ShowContextMenu(const QPoint& point, QPlainTextEdit* text_edit);
    void ApplyLineTransformation(KaraokeData::SongPosition start_position,
                KaraokeData::SongPosition end_position, bool split_syllables_at_start_and_end,
                std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f);
    void ApplyLineTransformation(bool split_syllables_at_start_and_end,
                std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f);

    void GoTo(SyllablePosition position);
    int TextPositionToLine(int position) const;
    SyllablePosition TextPositionToSyllable(int position) const;
    int TextPositionFromSyllable(SyllablePosition position) const;

    SyllablePosition GetPreviousSyllable() const;
    SyllablePosition GetPreviousSyllable(SyllablePosition pos) const;
    SyllablePosition GetCurrentSyllable() const;
    SyllablePosition GetNextSyllable() const;
    SyllablePosition GetPreviousLine() const;
    SyllablePosition GetNextLine() const;
    KaraokeData::Syllable* GetSyllable(SyllablePosition pos) const;
    KaraokeData::SongPosition ToSongPosition(int position, Mode mode) const;

    // For lines which are selected by the user in their entirety, a direct pointer
    // to the line is added to the result vector. For lines which are partially selected
    // by the user, a copy of the selected part of the line is stored in one of the out
    // parameters, and a pointer to that new copy is added to the result vector.
    QVector<const KaraokeData::Line*> GetSelectedLines(
            KaraokeData::ReadOnlyLine* partial_first_line_out,
            KaraokeData::ReadOnlyLine* partial_last_line_out);

    QPlainTextEdit* GetActiveTextEdit();

    KaraokeData::Centiseconds GetLatencyCompensatedTime() const;

    TimingEventFilter m_timing_event_filter;

    TextEventFilter m_text_event_filter;
    QPlainTextEdit* m_raw_text_edit;
    QPlainTextEdit* m_rich_text_edit;

    std::vector<std::unique_ptr<LineTimingDecorations>> m_line_timing_decorations;

    std::chrono::milliseconds m_time = std::chrono::milliseconds(-1);
    double m_speed = 0.0;

    Mode m_mode;

    bool m_raw_updates_disabled = false;
    bool m_rich_updates_disabled = false;
    bool m_rich_cursor_updates_disabled = false;

    KaraokeData::Song* m_song_ref;
};
