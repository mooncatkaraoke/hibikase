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

#include <memory>
#include <utility>

#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include "KaraokeContainer/Container.h"
#include "KaraokeContainer/PlainContainer.h"
#include "KaraokeData/Song.h"

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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

    std::unique_ptr<KaraokeContainer::Container> container =
            KaraokeContainer::Load(load_path.toUtf8().constData());
    m_song = KaraokeData::Load(container->ReadLyricsFile());

    if (!m_song->IsEditable())
    {
        // TODO: Add a way to create a Soramimi/MoonCat song instead of having to use Load
        std::unique_ptr<KaraokeData::Song> converted_song = KaraokeData::Load({});
        for (KaraokeData::Line* line : m_song->GetLines())
            converted_song->AddLine(line->GetSyllables());
        m_song = std::move(converted_song);
    }

    // TODO: Encoding
    ui->plainTextEdit->setPlainText(QString::fromUtf8(m_song->GetRaw().c_str()));
}

void MainWindow::on_actionSave_As_triggered()
{
    if (!m_song)
        return;

    // TODO: Encoding
    QByteArray data = ui->plainTextEdit->toPlainText().toUtf8();
    // TODO: Copying here is inefficient. We should be updating the Song live anyway
    m_song = KaraokeData::Load(std::vector<char>(data.constBegin(), data.constEnd()));
    QString save_path = QFileDialog::getSaveFileName(this);
    KaraokeContainer::PlainContainer::SaveLyricsFile(save_path.toUtf8().constData(),
                                                     m_song->GetRaw());
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
