// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include <QMainWindow>

#include "KaraokeContainer/Container.h"
#include "KaraokeData/Song.h"
#include "AudioFile.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

signals:
    void SongReplaced(KaraokeData::Song* song);

protected:
    void closeEvent(QCloseEvent* event);

private slots:
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_As_triggered();
    void on_actionAbout_Hibikase_triggered();

    void OnSongModified();

private:
    const QString LOAD_FILTER = QStringLiteral("All Lyric Files (*.txt *.vsqx);;All Files (*.*)");
    const QString SAVE_FILTER = QStringLiteral("Soramimi Lyrics (*.txt);;All Files (*.*)");

    void UpdateWindowTitle();
    void LoadAudio();
    bool Save(QString path);
    bool SaveAs();
    bool SaveUnsavedChanges();

    Ui::MainWindow* ui;

    std::unique_ptr<KaraokeContainer::Container> m_container;
    std::unique_ptr<KaraokeData::Song> m_song;
    QString m_save_path;
    bool m_unsaved_changes = false;
};
