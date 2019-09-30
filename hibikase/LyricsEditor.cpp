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

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>

#include <QAction>
#include <QFont>
#include <QMenu>
#include <QTextCursor>
#include <QVBoxLayout>

#include "LyricsEditor.h"
#include "TextTransform/RomanizeHangul.h"
#include "TextTransform/Syllabify.h"

static QFont WithPointSize(QFont font, qreal size)
{
    // For historical reasons,[0] points are one-third bigger on Windows than
    // on macOS. Qt doesn't account for this, so we must.
    // [0] https://blogs.msdn.microsoft.com/fontblog/2005/11/08/where-does-96-dpi-come-from-in-windows/
#ifdef TARGET_OS_MAC
    size *= 96.0 / 72.0;
#endif
    font.setPointSizeF(size);
    return font;
}

LyricsEditor::LyricsEditor(QWidget* parent) : QWidget(parent)
{
    m_raw_text_edit = new QPlainTextEdit();
    m_rich_text_edit = new QPlainTextEdit();

    connect(m_raw_text_edit->document(), &QTextDocument::contentsChange,
            this, &LyricsEditor::OnRawContentsChange);

    m_raw_text_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_raw_text_edit, &QPlainTextEdit::customContextMenuRequested,
            this, &LyricsEditor::ShowContextMenu);

    m_rich_text_edit->setReadOnly(true);

    m_raw_text_edit->setTabChangesFocus(true);
    m_rich_text_edit->setTabChangesFocus(true);

    m_raw_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_rich_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);

    m_rich_text_edit->setFont(WithPointSize(QFont(), 14));
    m_raw_text_edit->setFont(WithPointSize(QFont(), 9));

    SetMode(Mode::Text);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setMargin(0);
    main_layout->addWidget(m_raw_text_edit);
    main_layout->addWidget(m_rich_text_edit);

    setLayout(main_layout);
}

void LyricsEditor::RebuildSong()
{
    ReloadSong(m_song_ref);
}

void LyricsEditor::ReloadSong(KaraokeData::Song* song)
{
    m_song_ref = song;

    m_updates_disabled = true;
    m_raw_text_edit->setPlainText(song->GetRaw());
    m_rich_text_edit->setPlainText(song->GetText());
    m_updates_disabled = false;

    const QVector<KaraokeData::Line*> lines = song->GetLines();
    m_line_timing_decorations.clear();
    m_line_timing_decorations.reserve(lines.size());
    int i = 0;
    for (KaraokeData::Line* line : lines)
    {
        auto decorations = std::make_unique<LineTimingDecorations>(line, i, m_rich_text_edit);
        decorations->Update(m_time);
        m_line_timing_decorations.emplace_back(std::move(decorations));

        i += line->GetText().size();
        i++;  // For the newline character
    }
}

void LyricsEditor::UpdateTime(std::chrono::milliseconds time)
{
    for (auto& decorations : m_line_timing_decorations)
        decorations->Update(time);

    m_time = time;
}

void LyricsEditor::SetMode(Mode mode)
{
    if (mode == Mode::Raw && m_mode != Mode::Raw)
    {
        const QTextCursor cursor = m_rich_text_edit->textCursor();
        QTextCursor end_cursor = m_rich_text_edit->textCursor();
        end_cursor.movePosition(QTextCursor::End);

        QTextCursor raw_cursor = m_raw_text_edit->textCursor();
        if (cursor == end_cursor)
        {
            raw_cursor.movePosition(QTextCursor::End);
        }
        else
        {
            int position = m_rich_text_edit->textCursor().position();
            auto it = std::upper_bound(m_line_timing_decorations.cbegin(), m_line_timing_decorations.cend(),
                                       position, [](int pos, auto& line) { return pos < line->GetPosition(); });
            it--;
            const int line = it - m_line_timing_decorations.cbegin();
            const int line_pos = (*it)->GetPosition();
            const int raw_position = m_song_ref->PositionToRaw(
                                     KaraokeData::SongPosition{line, position - line_pos});

            raw_cursor.setPosition(raw_position);
        }
        m_raw_text_edit->setTextCursor(raw_cursor);
    }
    if (mode != Mode::Raw && m_mode == Mode::Raw)
    {
        const int position = m_raw_text_edit->textCursor().position();
        const KaraokeData::SongPosition song_position = m_song_ref->PositionFromRaw(position);
        QTextCursor cursor = m_rich_text_edit->textCursor();
        if (static_cast<size_t>(song_position.line) < m_line_timing_decorations.size())
        {
            const int line_position = m_line_timing_decorations[song_position.line]->GetPosition();
            cursor.setPosition(line_position + song_position.position_in_line);
        }
        else
        {
            cursor.movePosition(QTextCursor::End);
        }
        m_rich_text_edit->setTextCursor(cursor);
    }

    m_mode = mode;

    switch (mode)
    {
    case Mode::Timing:
        m_raw_text_edit->setVisible(false);
        m_rich_text_edit->setVisible(true);
        m_rich_text_edit->setTextInteractionFlags(Qt::NoTextInteraction);
        break;
    case Mode::Text:
        m_raw_text_edit->setVisible(false);
        m_rich_text_edit->setVisible(true);
        m_rich_text_edit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        break;
    case Mode::Raw:
        m_raw_text_edit->setVisible(true);
        m_rich_text_edit->setVisible(false);
        break;
    }
}

void LyricsEditor::OnRawContentsChange(int position, int chars_removed, int chars_added)
{
    if (!m_updates_disabled)
    {
        m_song_ref->UpdateRawText(position, chars_removed,
                                  m_raw_text_edit->toPlainText().midRef(position, chars_added));
    }
}

void LyricsEditor::ShowContextMenu(const QPoint& point)
{
    bool has_selection = m_raw_text_edit->textCursor().hasSelection();
    has_selection = true; // TODO: Remove this line once the selection actually is used

    QMenu* menu = m_raw_text_edit->createStandardContextMenu(point);
    menu->addSeparator();
    QMenu* syllabify = menu->addMenu(QStringLiteral("Syllabify"));
    syllabify->setEnabled(has_selection);
    syllabify->addAction(QStringLiteral("Basic"), this, SLOT(SyllabifyBasic()));
    menu->addAction(QStringLiteral("Romanize Hangul"), this,
                    SLOT(RomanizeHangul()))->setEnabled(has_selection);

    menu->exec(m_raw_text_edit->mapToGlobal(point));

    delete menu;
}

void LyricsEditor::SyllabifyBasic()
{
    // TODO: Only use the selection, not the whole document
    /*QTextCursor cursor = m_raw_text_edit->textCursor();
    int start = cursor.position();
    int end = cursor.anchor();*/

    RebuildSong();

    for (KaraokeData::Line* line : m_song_ref->GetLines())
        line->SetSyllableSplitPoints(TextTransform::SyllabifyBasic(line->GetText()));

    m_raw_text_edit->setPlainText(m_song_ref->GetRaw());
}

void LyricsEditor::RomanizeHangul()
{
    // TODO: Only use the selection, not the whole document

    RebuildSong();

    for (KaraokeData::Line* line : m_song_ref->GetLines())
        TextTransform::RomanizeHangul(line);

    m_raw_text_edit->setPlainText(m_song_ref->GetRaw());
}
