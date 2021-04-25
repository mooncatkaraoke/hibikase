// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

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
    void ApplyLineTransformation(int start_line, int end_line,
                                 std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f);
    void ApplyLineTransformation(std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f);

    void GoTo(SyllablePosition position);
    int TextPositionToLine(int position) const;
    SyllablePosition TextPositionToSyllable(int position) const;
    int TextPositionFromSyllable(SyllablePosition position) const;

    LyricsEditor::SyllablePosition GetPreviousSyllable() const;
    LyricsEditor::SyllablePosition GetPreviousSyllable(SyllablePosition pos) const;
    LyricsEditor::SyllablePosition GetCurrentSyllable() const;
    LyricsEditor::SyllablePosition GetNextSyllable() const;
    LyricsEditor::SyllablePosition GetPreviousLine() const;
    LyricsEditor::SyllablePosition GetNextLine() const;
    KaraokeData::Syllable* GetSyllable(SyllablePosition pos) const;

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
