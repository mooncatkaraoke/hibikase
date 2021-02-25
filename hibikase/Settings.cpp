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

#include "Settings.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QTextCodec>

QString Settings::GetDataPath()
{
    const QString path(QStringLiteral("data/"));

#ifdef Q_OS_DARWIN
    if (!QDir(path).exists())
        return QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources/");
#endif

    return path;
}

QTextCodec* Settings::GetLoadCodec(const QByteArray& data)
{
    QTextCodec::ConverterState state;
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    const QString text = codec->toUnicode(data.constData(), data.size(), &state);

    // Fallback if the text isn't valid UTF-8
    if (state.invalidChars > 0)
        codec = QTextCodec::codecForName("Windows-1252");

    return codec;
}

QTextCodec* Settings::GetSaveCodec()
{
    return QTextCodec::codecForName("UTF-8");
}

Setting<qreal> Settings::timing_text_font_size{"TimingTextFontSize", "Font size in Timing/Text view", 14};
Setting<qreal> Settings::raw_font_size{"RawFontSize", "Font size in Raw view", 9};

Setting<qreal>* const Settings::REAL_SETTINGS[] = {
    &timing_text_font_size,
    &raw_font_size,
};
