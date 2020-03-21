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

#include "TextTransform/Syllabify.h"

#include <functional>
#include <memory>

#include <QChar>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QStringRef>
#include <QTextStream>
#include <QVector>

#include "KaraokeData/Song.h"
#include "KaraokeData/ReadOnlySong.h"
#include "Settings.h"
#include "TextTransform/HangulUtils.h"

namespace TextTransform
{
static const QString::NormalizationForm NORMALIZATION_FORM = QString::NormalizationForm_KD;

Syllabifier::Syllabifier(const QString& language_code)
    : m_locale(language_code)
{
    BuildPatterns(language_code);
}

void Syllabifier::BuildPatterns(const QString& language_code)
{
    m_patterns.push_back(QMap<QString, QString>());
    m_max_pattern_size.push_back(0);

    QFile file(Settings::GetDataPath() + QStringLiteral("syllabification/") +
               language_code + QStringLiteral(".txt"));
    if (!file.open(QIODevice::ReadOnly))
        return;

    QTextStream in(&file);
    in.setCodec("UTF-8");
    while (!in.atEnd())
        BuildPattern(in.readLine().normalized(NORMALIZATION_FORM));

    file.close();
}

void Syllabifier::BuildPattern(const QString& line, int i)
{
    while (i < line.size() && line[i].isSpace())
        ++i;

    if (i >= line.size() || line[i] == '#' || line[i] == '%')
        return;

    if (line[i].isUpper())
    {
        if (line == QStringLiteral("NEXTLEVEL"))
        {
            m_patterns.push_back(QMap<QString, QString>());
            m_max_pattern_size.push_back(0);
        }

        return;
    }

    int letters = 0;
    int line_size = line.size();
    for (int j = i; j < line_size; ++j)
    {
        if (line[j] == '/')
        {
            // A slash in the middle of a line indicates the start of a Hunspell Hyphen non-standard
            // hyphenation pattern (e.g. tillåta -> till-låta). Since we are not doing hyphenation,
            // we just treat the line as if the slash and everything after it doesn't exist.
            line_size = j;
        }
        else if (line[j].isSpace() || line[j] == '#' || line[j] == '%')
        {
            // In TeX files but seemingly not Hunspell Hyphen files, spaces can be used to separate
            // patterns on the same line, and a comment can start in the middle of a line.
            line_size = j;
            BuildPattern(line, j);
        }
        else if (line[j] < '0' || line[j] > '9')
        {
            letters++;
        }
    }

    QString key(letters, QChar());
    QString value(letters + 1, QChar('0'));

    for (int j = i, k = 0; j < line_size; ++j)
    {
        if (line[j] < '0' || line[j] > '9')
            key[k++] = line[j];
        else
            value[k] = line[j];
    }

    const auto it = m_patterns.back().find(key);
    if (it != m_patterns.back().end())
    {
        const QString other_value = *it;
        for (int i = 0; i < value.size(); ++i)
            value[i] = std::max<QChar>(value[i], other_value[i]);
    }

    m_patterns.back().insert(key, value);
    m_max_pattern_size.back() = std::max(m_max_pattern_size.back(), key.size());
}

static bool IsHighSurrogate(const QString& text, int i)
{
    return text[i].isHighSurrogate() && i + 1 < text.size() && text[i + 1].isLowSurrogate();
}

static bool IsLowSurrogate(const QString& text, int i)
{
    return text[i].isLowSurrogate() && i > 0 && text[i - 1].isHighSurrogate();
}

// For surrogate pairs, i must point to the second half (the low surrogate),
// otherwise the result will be U+FFFD.
static uint CodepointFromUTF16(const QString& text, int i)
{
    if (text[i].isHighSurrogate())
        return 0xFFFD;

    if (!IsLowSurrogate(text, i))
        return text[i].unicode();

    return QChar::surrogateToUcs4(text[i - 1], text[i]);
}

// Same as above, except returns the codepoint after i, not at i.
// The size of the string must be at least i + 2.
static uint NextCodepointFromUTF16(const QString& text, int i)
{
    return CodepointFromUTF16(text, i + (IsHighSurrogate(text, i + 1) ? 2 : 1));
}

static bool IsLetterOrNumber(const QString& text, int i, uint codepoint)
{
    if (QChar::isLetterOrNumber(codepoint))
        return true;

    // Treat apostrophes as letters, so that words like "isn't" won't get split at
    // inappropriate locations. However, don't treat apostrophes as letters if they aren't
    // surrounded by letters on both sides, since they then likely are used as quotation marks.
    if ((codepoint == U'\'' || codepoint == U'’') && i > 0 && i + 1 < text.size())
    {
        const uint prev_codepoint = CodepointFromUTF16(text, i - 1);

        // Ideally, the case where the previous codepoint is a mark would also include
        // a check that the mark is preceded by a letter or number (with 0 or more marks
        // in between), but that would be more complicated code for very little gain...
        // Punctuation and spaces normally don't have combining marks attached.
        if ((QChar::isLetterOrNumber(prev_codepoint) || QChar::isMark(prev_codepoint)) &&
            QChar::isLetterOrNumber(NextCodepointFromUTF16(text, i)))
        {
            return true;
        }
    }

    return false;
}

static bool ModifiesPreviousKana(const QChar character)
{
    const uint codepoint = character.unicode();
    if (codepoint >= 0x3040 && codepoint <= 0x30FF)
    {
        const uint c = codepoint - (codepoint >= 0x30A0 ? 0x30A0 : 0x3040);
        return c == 0x01 || c == 0x03 || c == 0x05 || c == 0x07 || c == 0x09 ||
               c == 0x43 || c == 0x45 || c == 0x47 || c == 0x4E;
    }

    return (codepoint >= 0x31F0 && codepoint <= 0x31FF) ||
           (codepoint >= 0xFF67 && codepoint <= 0xFF6F);
}

static QChar::Script DetermineScript(const QString& text, int start, int end)
{
    // We assume that the word cannot contain multiple explicit scripts, due to processing
    // done earlier. It can however mix an explicit script with an implicit script such as
    // Script_Common (e.g. if we have kana followed by U+30FC KATAKANA-HIRAGANA PROLONGED
    // SOUND MARK). Therefore, we can't just check a single character of the word, since it
    // might be of Script_Common or Script_Inherited instead of the actual script.

    for (int i = start; i < end; ++i)
    {
        if (IsHighSurrogate(text, i))
            continue;

        const uint codepoint = CodepointFromUTF16(text, i);

        if (QChar::isMark(codepoint))
            continue;

        const QChar::Script script = QChar::script(codepoint);
        if (script > 2)
            return script;
    }

    return QChar::Script_Unknown;
}

static void SyllabifyWordSimple(QVector<int>* split_points, const QString& text, int start, int end,
                                std::function<bool(const QString&, int)> split_predicate)
{
    for (int i = start; i < end - 1; ++i)
    {
        if (IsHighSurrogate(text, i))
        {
        }
        else if (QChar::isMark(CodepointFromUTF16(text, i)))
        {
            if (split_points->back() == i - (IsLowSurrogate(text, i) ? 2 : 1))
                (*split_points)[split_points->size() - 1] = i;
        }
        else if (split_predicate(text, i))
        {
            split_points->append(i + 1);
        }
    }
}

QString Syllabifier::ApplyPatterns(QStringRef word, int level) const
{
    const QString wrapped_word = QChar('.') + word + QChar('.');
    QString splits(word.size() - 1, QChar('0'));

    for (int i = 0; i < wrapped_word.size(); ++i)
    {
        for (int j = 1; j <= wrapped_word.size() - i && j <= m_max_pattern_size[level]; ++j)
        {
            if (i + j < wrapped_word.size() && QChar::isMark(NextCodepointFromUTF16(wrapped_word, i + j - 1)))
                continue;  // A pattern that ends with "a" must not match "ä"

            const auto it = m_patterns[level].find(wrapped_word.mid(i, j));
            if (it != m_patterns[level].end())
            {
                const QString& pattern = it.value();
                for (int k = 0; k < pattern.size(); ++k)
                {
                    const int l = i - 2 + k;
                    if (l >= 0 && l < splits.size())
                        splits[l] = std::max<QChar>(splits[l], pattern[k]);
                }
            }
        }
    }

    if (level + 1 < m_patterns.size())
    {
        int subword_start = 0;

        for (int i = 0; i < splits.size(); ++i)
        {
            if (splits[i].unicode() % 2 == 1)
            {
                splits.replace(subword_start, i - subword_start,
                               ApplyPatterns(word.mid(subword_start, i + 1 - subword_start), level));
                subword_start = i + 1;
            }
        }

        if (subword_start == 0)
        {
            return ApplyPatterns(word, level + 1);
        }
        else
        {
            splits.replace(subword_start, splits.size() - subword_start,
                           ApplyPatterns(word.mid(subword_start, word.size() - subword_start), level));
        }
    }

    return splits;
}

// Adds split points inside a word, but not at the beginning or end.
// Expects a word to contain 1 or more letters, 0 or more marks, and no other
// character categories. The letters must not make use of more than one explicit script.
void Syllabifier::SyllabifyWord(QVector<int>* split_points, const QString& text, int start, int end) const
{
    switch (DetermineScript(text, start, end))
    {
    case QChar::Script_Hangul:
        SyllabifyWordSimple(split_points, text, start, end, IsHangulSyllableEnd);
        break;

    case QChar::Script_Hiragana:
    case QChar::Script_Katakana:
        SyllabifyWordSimple(split_points, text, start, end, [](const QString& text, int i) {
            return !ModifiesPreviousKana(text[i + 1]);
        });
        break;

    case QChar::Script_Han:
    case QChar::Script_Yi:
    case QChar::Script_Tangut:
    case QChar::Script_Nushu:
        SyllabifyWordSimple(split_points, text, start, end, [](const QString&, int) {
            return true;
        });
        break;

    default:
        const QString word = text.mid(start, end - start);

        QVector<int> index_mapping;
        for (int i = 0; i < word.size();)
        {
            const int size = IsHighSurrogate(text, i) ? 2 : 1;

            const QString normalized = m_locale.toLower(word.mid(i, size).normalized(NORMALIZATION_FORM));
            for (int j = 0; j < normalized.size(); ++j)
                index_mapping.append(start + i);

            i += size;
        }

        const QString normalized_word = m_locale.toLower(word).normalized(NORMALIZATION_FORM);
        if (index_mapping.size() != normalized_word.size())
        {
            QMessageBox::critical(nullptr, QStringLiteral("Error"),
                                  QStringLiteral("Unexpected normalization length in word \"%1\"").arg(word));
        }

        const QString splits = ApplyPatterns(QStringRef(&normalized_word));
        for (int i = 0; i < splits.size(); ++i)
        {
            if (splits[i].unicode() % 2 == 1)
                split_points->append(index_mapping[i + 1]);
        }
        break;
    }
}

QVector<int> Syllabifier::Syllabify(const QString& text) const
{
    QVector<int> split_points;

    if (text.isEmpty())
        return split_points;

    split_points.append(0);

    QChar::Script previous_script = QChar::Script_Unknown;
    bool last_was_letter_or_number = false;
    bool last_was_number = false;
    int word_pre_start = 0;
    int word_start = 0;
    for (int i = 0; i < text.size(); ++i)
    {
        if (IsHighSurrogate(text, i))
            continue;

        const uint codepoint = CodepointFromUTF16(text, i);

        if (QChar::isMark(codepoint))
            continue;

        const QChar::Script script = QChar::script(codepoint);
        const bool is_letter_or_number = IsLetterOrNumber(text, i, codepoint);
        const bool is_number = QChar::isNumber(codepoint);
        const bool scripts_match = is_number == last_was_number &&
                (previous_script == script || script <= 2 || previous_script <= 2);

        const int index_of_current_codepoint = IsLowSurrogate(text, i) ? i - 1 : i;

        if ((!is_letter_or_number || !scripts_match) && last_was_letter_or_number)
        {
            if (word_pre_start < 0)
                split_points.append(word_start);
            else if (word_pre_start != 0)
                split_points.append(word_pre_start);

            if (!last_was_number)
                SyllabifyWord(&split_points, text, word_start, index_of_current_codepoint);

            word_pre_start = -1;
        }

        if (!last_was_letter_or_number || !scripts_match)
            word_start = index_of_current_codepoint;

        if (QChar::isSpace(codepoint))
            word_pre_start = i + 1;

        previous_script = script;
        last_was_letter_or_number = is_letter_or_number;
        last_was_number = is_number;
    }

    if (last_was_letter_or_number)
    {
        if (word_pre_start < 0)
            split_points.append(word_start);
        else if (word_pre_start != 0)
            split_points.append(word_pre_start);

        SyllabifyWord(&split_points, text, word_start, text.size());
    }

    split_points.append(text.size());

    return split_points;
}

std::unique_ptr<KaraokeData::Line> Syllabifier::Syllabify(const KaraokeData::Line& line) const
{
    const QString line_text = line.GetText();
    std::unique_ptr<KaraokeData::ReadOnlyLine> new_line = std::make_unique<KaraokeData::ReadOnlyLine>();
    if (line_text.isEmpty())
        return std::move(new_line);

    const QVector<int> split_points = Syllabify(line_text);
    new_line->m_syllables.reserve(split_points.size() - 1);
    for (int i = 1; i < split_points.size(); ++i)
    {
        new_line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(
                line_text.mid(split_points[i - 1], split_points[i] - split_points[i - 1]),
                KaraokeData::PLACEHOLDER_TIME, KaraokeData::PLACEHOLDER_TIME));
    }

    return std::move(new_line);
}

QVector<QString> Syllabifier::AvailableLanguages()
{
    QVector<QString> result;

    QDirIterator iterator(Settings::GetDataPath() + QStringLiteral("syllabification/"));
    while (iterator.hasNext())
    {
        iterator.next();
        QFileInfo file_info = iterator.fileInfo();
        if (file_info.isFile() && file_info.suffix() == QStringLiteral("txt"))
            result.push_back(file_info.baseName());
    }

    return result;
}

}
