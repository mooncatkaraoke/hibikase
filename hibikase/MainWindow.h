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
