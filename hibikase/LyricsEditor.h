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
#include <memory>
#include <vector>

#include <QPlainTextEdit>
#include <QPoint>
#include <QWidget>

#include "KaraokeData/Song.h"

#include "LineTimingDecorations.h"

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

    explicit LyricsEditor(QWidget* parent = 0);

    void SetMode(Mode mode);

    // TODO: Get rid of the need for this function by continually updating the song
    void RebuildSong();

public slots:
    void ReloadSong(KaraokeData::Song* song);
    void UpdateTime(std::chrono::milliseconds time);

private slots:
    void ShowContextMenu(const QPoint& point);
    void SyllabifyBasic();
    void RomanizeHangul();

private:
    QPlainTextEdit* m_raw_text_edit;
    QPlainTextEdit* m_rich_text_edit;
    std::vector<std::unique_ptr<LineTimingDecorations>> m_line_timing_decorations;
    std::chrono::milliseconds m_time = std::chrono::milliseconds(-1);

    KaraokeData::Song* m_song_ref;
};
