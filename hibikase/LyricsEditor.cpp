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

#include <utility>

#include <QVBoxLayout>

#include "LyricsEditor.h"

LyricsEditor::LyricsEditor(QWidget *parent) : QWidget(parent)
{
    m_text_edit = new QPlainTextEdit();

    QVBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setMargin(0);
    main_layout->addWidget(m_text_edit);

    setLayout(main_layout);
}

void LyricsEditor::RebuildSong() {
    // TODO: We shouldn't be re-encoding here
    QByteArray data = m_text_edit->toPlainText().toUtf8();
    std::unique_ptr<KaraokeData::Song> new_song = KaraokeData::Load(data);

    m_song_ref->RemoveAllLines();
    for (KaraokeData::Line* line : new_song->GetLines())
        m_song_ref->AddLine(line->GetSyllables());
}

void LyricsEditor::ReloadSong(KaraokeData::Song* song) {
    m_song_ref = song;
    m_text_edit->setPlainText(song->GetRaw());
}
