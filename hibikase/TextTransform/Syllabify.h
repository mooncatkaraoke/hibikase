// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// As an additional permission for this file only, you can (at your
// option) instead use this file under the terms of CC0.
// <http://creativecommons.org/publicdomain/zero/1.0/>

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>

#include <QLocale>
#include <QMap>
#include <QString>
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

private:
    void BuildPatterns(const QString& language_code);
    void BuildPattern(const QString& line, int i = 0);
    QString ApplyPatterns(const QString& word) const;
    void SyllabifyWord(QVector<int>* split_points, const QString& text, int start, int end) const;

    QLocale m_locale;
    QMap<QString, QString> m_patterns;
    int m_max_pattern_size = 0;
};

}
