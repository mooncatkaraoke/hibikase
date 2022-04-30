// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <memory>

#include <QLocale>
#include <QMap>
#include <QString>
#include <QStringRef>
#include <QVector>

#include "KaraokeData/Song.h"

namespace TextTransform
{

class Syllabifier final
{
public:
    Syllabifier(const QString& language_code);

    // Returns the syllable split points for a line of text
    QVector<int> Syllabify(const QString& text) const;

    std::unique_ptr<KaraokeData::Line> Syllabify(const KaraokeData::Line& line) const;

    static QVector<QString> AvailableLanguages();

private:
    void BuildPatterns(const QString& language_code);
    void BuildPattern(const QString& line, int i = 0);
    QString ApplyPatterns(QStringRef word, int level = 0) const;
    void SyllabifyWord(QVector<int>* split_points, const QString& text, int start, int end) const;

    QLocale m_locale;
    QVector<QMap<QString, QString>> m_patterns;
    QVector<int> m_max_pattern_size;
};

}
