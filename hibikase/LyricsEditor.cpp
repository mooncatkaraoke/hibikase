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

LyricsEditor::LyricsEditor(QWidget* parent) : QWidget(parent)
{
    m_raw_text_edit = new QPlainTextEdit();
    m_rich_text_edit = new QPlainTextEdit();

    m_raw_text_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_raw_text_edit, &QPlainTextEdit::customContextMenuRequested,
            this, &LyricsEditor::ShowContextMenu);

    m_rich_text_edit->setReadOnly(true);

    m_raw_text_edit->setTabChangesFocus(true);
    m_rich_text_edit->setTabChangesFocus(true);

    m_raw_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_rich_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont large_font;
    large_font.setPointSize(14);
    m_rich_text_edit->setFont(large_font);
    QFont small_font;
    small_font.setPointSize(9);
    m_raw_text_edit->setFont(small_font);

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setMargin(0);
    main_layout->addWidget(m_raw_text_edit);
    main_layout->addWidget(m_rich_text_edit);

    setLayout(main_layout);
}

void LyricsEditor::RebuildSong()
{
    // TODO: We shouldn't be re-encoding here
    QByteArray data = m_raw_text_edit->toPlainText().toUtf8();
    std::unique_ptr<KaraokeData::Song> new_song = KaraokeData::Load(data);

    m_song_ref->RemoveAllLines();
    for (KaraokeData::Line* line : new_song->GetLines())
        m_song_ref->AddLine(line->GetSyllables(), line->GetPrefix(), line->GetSuffix());

    ReloadSong(m_song_ref);
}

void LyricsEditor::ReloadSong(KaraokeData::Song* song)
{
    m_song_ref = song;

    m_raw_text_edit->setPlainText(song->GetRaw());
    m_rich_text_edit->setPlainText(song->GetText());

    const QVector<KaraokeData::Line*> lines = song->GetLines();
    m_line_timing_decorations.clear();
    m_line_timing_decorations.reserve(lines.size());
    size_t i = 0;
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
