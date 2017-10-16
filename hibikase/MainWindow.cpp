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

#include <QFileDialog>
#include <QMessageBox>
#include <QRadioButton>
#include <QString>

#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"
#include "KaraokeData/Song.h"

#include "LyricsEditor.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->timeLabel->setTextFormat(Qt::PlainText);

    connect(this, &MainWindow::SongReplaced, ui->mainLyrics, &LyricsEditor::ReloadSong);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::UpdateTime);

    connect(ui->timingRadioButton, &QRadioButton::toggled, [this](bool checked) {
        if (checked)
            ui->mainLyrics->SetMode(LyricsEditor::Mode::Timing);
    });
    connect(ui->textRadioButton, &QRadioButton::toggled, [this](bool checked) {
        if (checked)
            ui->mainLyrics->SetMode(LyricsEditor::Mode::Text);
    });
    connect(ui->rawRadioButton, &QRadioButton::toggled, [this](bool checked) {
        if (checked)
            ui->mainLyrics->SetMode(LyricsEditor::Mode::Raw);
    });
    ui->textRadioButton->setChecked(true);

    // TODO: Add a way to create a Soramimi/MoonCat song instead of having to use Load
    m_song = KaraokeData::Load({});
    emit SongReplaced(m_song.get());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    QString load_path = QFileDialog::getOpenFileName(this);
    if (load_path.isEmpty())
        return;

    std::unique_ptr<KaraokeContainer::Container> container = KaraokeContainer::Load(load_path);
    std::unique_ptr<KaraokeData::Song> song = KaraokeData::Load(container->ReadLyricsFile());

    if (!song->IsEditable())
    {
        // TODO: Add a way to create a Soramimi/MoonCat song instead of having to use Load
        std::unique_ptr<KaraokeData::Song> converted_song = KaraokeData::Load({});
        for (KaraokeData::Line* line : song->GetLines())
            converted_song->AddLine(line->GetSyllables());
        m_song = std::move(converted_song);
    }
    else
    {
        m_song = std::move(song);
    }

    emit SongReplaced(m_song.get());
}

void MainWindow::on_actionSave_As_triggered()
{
    QString save_path = QFileDialog::getSaveFileName(this);
    ui->mainLyrics->RebuildSong();
    KaraokeContainer::PlainContainer::SaveLyricsFile(save_path, m_song->GetRawBytes());
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionAbout_Hibikase_triggered()
{
    QMessageBox::about(this, "Hibikase",
                       "Hibikase, a karaoke editor by MoonCat Karaoke\n"
                       "\n"
                       "This program is free software: you can redistribute it and/or modify "
                       "it under the terms of the GNU General Public License as published by "
                       "the Free Software Foundation, either version 2 of the License, or "
                       "(at your option) any later version.\n"
                       "\n"
                       "This program is distributed in the hope that it will be useful, "
                       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
                       "GNU General Public License for more details.\n"
                       "\n"
                       "You should have received a copy of the GNU General Public License "
                       "along with this program. If not, see <http://www.gnu.org/licenses/>.");
}

void MainWindow::on_playButton_clicked()
{
    m_is_playing = !m_is_playing;

    if (m_is_playing)
    {
        m_playback_timer.start();
        m_timer->start(10);  // TODO: Can this be done every frame instead?

        ui->playButton->setText(QStringLiteral("Stop"));
    }
    else
    {
        m_timer->stop();

        // TODO: This string is also in the UI file. Can it be deduplicated?
        ui->playButton->setText(QStringLiteral("Play"));
    }

    UpdateTime();
}

void MainWindow::UpdateTime()
{
    QString text;
    qint64 ms = -1;
    if (m_is_playing)
    {
        ms = m_playback_timer.elapsed();
        text = QStringLiteral("%1:%2:%3").arg(ms / 60000,     2, 10, QChar('0'))
                                         .arg(ms / 1000 % 60, 2, 10, QChar('0'))
                                         .arg(ms / 10 % 100,  2, 10, QChar('0'));
    }
    ui->mainLyrics->UpdateTime(std::chrono::milliseconds(ms));
    ui->timeLabel->setText(text);
}
