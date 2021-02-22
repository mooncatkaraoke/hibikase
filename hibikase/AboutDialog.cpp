#include "AboutDialog.h"

#include <algorithm>
#include <utility>
#include <vector>

#include <QChar>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QTextCodec>
#include <QTextStream>
#include <QVBoxLayout>

#include "Settings.h"

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("About Hibikase"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QString git_version(GIT_VERSION);
    if (!git_version.isEmpty())
        git_version.prepend(QStringLiteral("<br>"));

    QLabel* logo_label = new QLabel(this);
    logo_label->setPixmap(QPixmap(":/logo.png"));

    QLabel* label = new QLabel(QStringLiteral(
                "<p><b>Hibikase, a karaoke editor by MoonCat Karaoke%1</b></p>\n"
                "\n"
                "<p>This program is free software: you can redistribute it and/or modify it under "
                "the terms of the GNU General Public License as published by the Free Software "
                "Foundation, either version 2 of the License, or (at your option) any later "
                "version.</p>\n"
                "\n"
                "<p>This program is distributed in the hope that it will be useful, but WITHOUT "
                "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR "
                "A PARTICULAR PURPOSE. See the GNU General Public License for more details.</p>"
                ).arg(git_version));
    label->setWordWrap(true);

    QHBoxLayout* top_layout = new QHBoxLayout();
    top_layout->addWidget(logo_label);
    top_layout->addWidget(label);

    QFrame* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);

    m_combo_box = new QComboBox();
    m_text_browser = new QTextBrowser();

    QPushButton* about_qt = new QPushButton(QStringLiteral("About &Qt"));
    QPushButton* close = new QPushButton(QStringLiteral("&Close"));

    QHBoxLayout* buttons_layout = new QHBoxLayout();
    buttons_layout->setAlignment(Qt::AlignRight);
    buttons_layout->addWidget(about_qt);
    buttons_layout->addWidget(close);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addLayout(top_layout);
    layout->addWidget(divider);
    layout->addWidget(m_combo_box);
    layout->addWidget(m_text_browser);
    layout->addLayout(buttons_layout);
    setLayout(layout);

    PopulateComboBox();

    connect(m_combo_box, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AboutDialog::OnCurrentIndexChanged);
    connect(about_qt, &QPushButton::released, [this]() { QMessageBox::aboutQt(this); });
    connect(close, &QPushButton::released, this, &QDialog::reject);

    OnCurrentIndexChanged(m_combo_box->currentIndex());
}

AboutDialog::~AboutDialog() = default;

QSize AboutDialog::sizeHint() const
{
    return {620, 600};
}

void AboutDialog::OnCurrentIndexChanged(int index)
{
    const QString path = m_combo_box->itemData(index).toString();

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;

    QTextStream stream(&file);
    stream.setCodec(QTextCodec::codecForName("UTF-8"));

    QChar comment_char('\0');
    bool first_line = true;
    QString text;
    while (!stream.atEnd())
    {
        const QString line = stream.readLine();

        if (first_line && !line.isEmpty() && (line.front() == QChar('%') || line.front() == QChar('#')))
            comment_char = line.front();

        if (comment_char != QChar('\0'))
        {
            // This case is for syllabification files

            if (line.isEmpty() || line.front() != comment_char)
                break;

            int begin = 0;
            int end = line.size();

            while (begin < end && line[begin] == comment_char)
                ++begin;
            while (end > begin && (line[end - 1] == comment_char || line[end - 1] == QChar(' ')))
                --end;
            if (begin < end && line[begin] == QChar(' '))
                ++begin;

            if (begin == end && first_line)
                continue;

            text += line.midRef(begin, end - begin);
        }
        else
        {
            // This case is for files in the license folder

            text += line;
        }

        text += '\n';

        first_line = false;
    }

    m_text_browser->setText(text);
}

void AboutDialog::PopulateComboBox()
{
    using Pair = std::pair<QString, QString>;

    std::vector<Pair> licenses;

    {
        QDirIterator iterator(QStringLiteral(":/licenses/"));
        while (iterator.hasNext())
        {
            iterator.next();
            QFileInfo file_info = iterator.fileInfo();
            licenses.emplace_back(file_info.completeBaseName(), file_info.filePath());
        }
    }

    {
        QDirIterator iterator(Settings::GetDataPath() + QStringLiteral("syllabification/"));
        while (iterator.hasNext())
        {
            iterator.next();
            QFileInfo file_info = iterator.fileInfo();
            if (file_info.isFile() && file_info.suffix() == QStringLiteral("txt"))
            {
                const QString title = QString("About syllabification file %1").arg(file_info.fileName());
                licenses.emplace_back(title, file_info.filePath());
            }
        }
    }

    std::sort(licenses.begin(), licenses.end(), [](const Pair& a, const Pair& b) {
        return a.first.localeAwareCompare(b.first) < 0;
    });

    for (const Pair& license : licenses)
    {
        m_combo_box->addItem(license.first, license.second);
        if (license.first.startsWith(QLatin1Literal("GNU General Public License")))
            m_combo_box->setCurrentIndex(m_combo_box->count() - 1);
    }
}
