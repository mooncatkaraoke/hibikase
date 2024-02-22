// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "SettingsDialog.h"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), m_keyboard_shortcuts_help{}
{
    ui->setupUi(this);

    connect(ui->menuEdit, &QMenu::aboutToShow, [this]() {
        ui->menuEdit->clear();
        ui->mainLyrics->AddActionsToMenu(ui->menuEdit);
        ui->menuEdit->addSeparator();
        ui->menuEdit->addAction(QStringLiteral("Preferences..."), [this]{
            SettingsDialog dialog(this);
            dialog.exec();
        });
    });

    connect(this, &MainWindow::SongReplaced, ui->mainLyrics, &LyricsEditor::ReloadSong);
    connect(this, &MainWindow::SongReplaced, ui->playbackWidget, &PlaybackWidget::ReloadSong);
    connect(ui->playbackWidget, &PlaybackWidget::TimeUpdated, ui->mainLyrics, &LyricsEditor::UpdateTime);
    connect(ui->playbackWidget, &PlaybackWidget::SpeedUpdated, ui->mainLyrics, &LyricsEditor::UpdateSpeed);
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

    QString load_path = QFileDialog::getOpenFileName(this, "Open lyrics file", QString(),
                                                     LOAD_FILTER);
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

void MainWindow::on_actionKeyboard_Shortcuts_triggered()
{
    if (m_keyboard_shortcuts_help)
    {
        m_keyboard_shortcuts_help->show();
        // Try to bring the window into focus if it's already open
        m_keyboard_shortcuts_help->raise();
        m_keyboard_shortcuts_help->activateWindow();
        return;
    }

    struct Shortcut
    {
        const char* name;
        const QKeySequence& key;
    };
    const std::initializer_list<Shortcut> TEXT_SHORTCUTS = {
        {"Create or delete syllable", LyricsEditor::TOGGLE_SYLLABLE},
    };
    const std::initializer_list<Shortcut> TIMING_SHORTCUTS = {
        {"Time syllable and advance", LyricsEditor::SET_SYLLABLE_START},
        {"Time end of previous syllable", LyricsEditor::SET_SYLLABLE_END_1},
        {"Go to previous syllable", LyricsEditor::PREVIOUS_SYLLABLE},
        {"Go to next syllable", LyricsEditor::NEXT_SYLLABLE},
        {"Go to start of previous line", LyricsEditor::PREVIOUS_LINE},
        {"Go to start of next line", LyricsEditor::NEXT_LINE},
    };

    QString text{};
    text.append("<table>");
    for (auto shortcuts : {
        std::make_pair("Text view", &TEXT_SHORTCUTS),
        std::make_pair("Timing view", &TIMING_SHORTCUTS),
    })
    {
        text.append("<tr>");
        text.append("<th width\"400\" colspan=\"2\">");
        text.append(shortcuts.first);
        text.append("</th>");
        text.append("</tr>");
        for (auto shortcut : *shortcuts.second)
        {
            text.append("<tr>");
            text.append("<td style=\"padding-right: 1em;\">");
            text.append(shortcut.name);
            text.append("</td>");
            text.append("<td>");
            text.append(shortcut.key.toString(QKeySequence::SequenceFormat::NativeText));
            text.append("</td>");
            text.append("</tr>");
        }
    }
    text.append("</table>");

    QMessageBox* box = new QMessageBox(this);
    box->setWindowTitle("Keyboard Shortcuts Help");
    box->setTextFormat(Qt::TextFormat::RichText);
    box->setInformativeText(text);
    box->setWindowModality(Qt::WindowModality::NonModal);
    box->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    box->show();

    m_keyboard_shortcuts_help = box;
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
    const QString save_path = QFileDialog::getSaveFileName(this, "Save lyrics file", QString(),
                                                           SAVE_FILTER);
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
