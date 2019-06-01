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

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#include "KaraokeData/Song.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

signals:
    void SongReplaced(KaraokeData::Song* song);

private slots:
    void on_actionOpen_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionAbout_Hibikase_triggered();
    void on_actionSave_As_triggered();

    void on_playButton_clicked();

    void UpdateTime();

private:
    Ui::MainWindow* ui;

    std::unique_ptr<KaraokeData::Song> m_song;

    QTimer* m_timer = new QTimer(this);
    QElapsedTimer m_playback_timer;
    bool m_is_playing = false;
};
