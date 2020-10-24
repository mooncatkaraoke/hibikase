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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <chrono>
#include <memory>
#include <utility>

#include <QFileDialog>
#include <QFileInfo>
#include <QMediaContent>
#include <QMessageBox>
#include <QRadioButton>
#include <QString>
#include <QBuffer>

#include "AboutDialog.h"
#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"
#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiSong.h"
#include "LyricsEditor.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(this, &MainWindow::SongReplaced, ui->mainLyrics, &LyricsEditor::ReloadSong);
    connect(ui->playbackWidget, &PlaybackWidget::TimeUpdated, ui->mainLyrics, &LyricsEditor::UpdateTime);
    connect(ui->mainLyrics, &LyricsEditor::Modified, this, &MainWindow::OnSongModified);

#ifndef Q_OS_MACOS
    // The macOS tab bar looks better with spacing between it and the editor,
    // whereas the Windows one looks best with no spacing and no extra border.
    ui->tabLayout->setSpacing(0);
    ui->modeTabBar->setDrawBase(false);
#endif
    ui->modeTabBar->setFocusPolicy(Qt::FocusPolicy::TabFocus);
    for (QString mode : {"&Timing", "Te&xt", "&Raw"})
    {
        ui->modeTabBar->addTab(mode);
    }
    connect(ui->modeTabBar, &QTabBar::currentChanged, [this](int index) {
        ui->mainLyrics->SetMode(static_cast<LyricsEditor::Mode>(index));
    });
    ui->modeTabBar->setCurrentIndex(static_cast<int>(LyricsEditor::Mode::Text));

    // TODO: Add a way to create a Soramimi song instead of having to use Load
    m_song = KaraokeData::Load({});
    emit SongReplaced(m_song.get());

    UpdateWindowTitle();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (SaveUnsavedChanges())
        event->accept();
    else
        event->ignore();
}

void MainWindow::on_actionNew_triggered()
{
    if (!SaveUnsavedChanges())
        return;

    // TODO: Add a way to create a Soramimi song instead of having to use Load
    m_song = KaraokeData::Load({});
    emit SongReplaced(m_song.get());

    m_container = nullptr;
    LoadAudio();

    m_save_path.clear();
    m_unsaved_changes = false;
    UpdateWindowTitle();
}

void MainWindow::on_actionOpen_triggered()
{
    if (!SaveUnsavedChanges())
        return;

    QString load_path = QFileDialog::getOpenFileName(this);
    if (load_path.isEmpty())
        return;

    m_container = KaraokeContainer::Load(load_path);
    std::unique_ptr<KaraokeData::Song> song = KaraokeData::Load(m_container->ReadLyricsFile());

    if (!song->IsEditable())
    {
        const KaraokeData::Song* const_song = static_cast<const KaraokeData::Song*>(song.get());
        m_song = std::make_unique<KaraokeData::SoramimiSong>(const_song->GetLines());
        m_save_path.clear();
    }
    else
    {
        m_song = std::move(song);
        m_save_path = load_path;
    }

    emit SongReplaced(m_song.get());
    LoadAudio();

    m_unsaved_changes = false;
    UpdateWindowTitle();
}

void MainWindow::on_actionSave_triggered()
{
    Save(m_save_path);
}

void MainWindow::on_actionSave_As_triggered()
{
    SaveAs();
}

void MainWindow::on_actionAbout_Hibikase_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::OnSongModified()
{
    m_unsaved_changes = true;
    UpdateWindowTitle();
}

void MainWindow::UpdateWindowTitle()
{
    QString file_name;
    if (m_unsaved_changes)
        file_name.append(QChar('*'));
    file_name.append(m_save_path.isEmpty() ? "Untitled" : QFileInfo(m_save_path).fileName());
    setWindowTitle(QStringLiteral("%1 - Hibikase").arg(file_name));
    setWindowFilePath(m_save_path); // macOS proxy icon support
}

void MainWindow::LoadAudio()
{
    ui->playbackWidget->LoadAudio(m_container ? m_container->ReadAudioFile() : nullptr);
}

bool MainWindow::Save(QString path)
{
    if (path.isEmpty())
        return SaveAs();

    m_container = KaraokeContainer::Load(path);
    if (!m_container->SaveLyricsFile(m_song->GetRawBytes()))
    {
        QMessageBox::warning(this, "Error", "The file could not be saved.");
        return false;
    }

    if (m_save_path != path)
    {
        m_save_path = path;
        LoadAudio();
    }

    m_unsaved_changes = false;
    UpdateWindowTitle();

    return true;
}

bool MainWindow::SaveAs()
{
    const QString save_path = QFileDialog::getSaveFileName(this);
    if (save_path.isEmpty())
        return false;

    return Save(save_path);
}

bool MainWindow::SaveUnsavedChanges()
{
    if (!m_unsaved_changes)
        return true;

    const QMessageBox::StandardButton result =
            QMessageBox::question(this, "Hibikase", "Do you want to save your changes?",
                                  QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (result == QMessageBox::Save)
        return Save(m_save_path);
    else if (result == QMessageBox::Discard)
        return true;
    else
        return false;
}
