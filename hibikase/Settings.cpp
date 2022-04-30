// SPDX-License-Identifier: GPL-2.0-or-later

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

Setting<int> Settings::audio_latency{"AudioLatency", "Audio latency (ms)", 0};
Setting<int> Settings::video_latency{"VideoLatency", "Video latency (ms)", 0};

Setting<qreal>* const Settings::REAL_SETTINGS[] = {
    &timing_text_font_size,
    &raw_font_size,
};

Setting<int>* const Settings::INT_SETTINGS[] = {
    &audio_latency,
    &video_latency,
};
