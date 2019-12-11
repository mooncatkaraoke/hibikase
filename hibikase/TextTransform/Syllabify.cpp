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

#include <functional>
#include <memory>

#include <QChar>
#include <QString>
#include <QVector>

#include "KaraokeData/Song.h"
#include "KaraokeData/ReadOnlySong.h"
#include "TextTransform/HangulUtils.h"
#include "TextTransform/Syllabify.h"

namespace TextTransform
{

// For surrogate pairs, i must point to the second half (the low surrogate),
// otherwise the result will be U+FFFD.
static uint CodepointFromUTF16(const QString& text, int i)
{
    if (text[i].isHighSurrogate())
        return 0xFFFD;

    if (!text[i].isLowSurrogate())
        return text[i].unicode();

    if (i == 0 || !text[i - 1].isHighSurrogate())
        return 0xFFFD;

    return QChar::surrogateToUcs4(text[i - 1], text[i]);
}

// Same as above, except returns the codepoint after i, not at i.
// The size of the string must be at least i + 2.
static uint NextCodepointFromUTF16(const QString& text, int i)
{
    return CodepointFromUTF16(text, i + (text.size() > i + 2 && text[i + 1].isHighSurrogate() ? 2 : 1));
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

static bool IsLetterOrNumber(const QString& text, int i)
{
    return IsLetterOrNumber(text, i, CodepointFromUTF16(text, i));
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
        if (text[i].isHighSurrogate())
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
        if (text[i].isHighSurrogate())
        {
        }
        else if (QChar::isMark(CodepointFromUTF16(text, i)))
        {
            if (split_points->back() == i - (text[i].isLowSurrogate() ? 2 : 1))
                (*split_points)[split_points->size() - 1] = i;
        }
        else if (split_predicate(text, i))
        {
            split_points->append(i + 1);
        }
    }
}

// Adds split points inside a word, but not at the beginning or end.
// Expects a word to contain 1 or more letters, 0 or more marks, and no other
// character categories. The letters must not make use of more than one explicit script.
static void SyllabifyWord(QVector<int>* split_points, const QString& text, int start, int end)
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
        break;
    }
}

QVector<int> SyllabifyBasic(const QString& text)
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
        if (text[i].isHighSurrogate())
            continue;

        const uint codepoint = CodepointFromUTF16(text, i);

        if (QChar::isMark(codepoint))
            continue;

        const QChar::Script script = QChar::script(codepoint);
        const bool is_letter_or_number = IsLetterOrNumber(text, i, codepoint);
        const bool is_number = QChar::isNumber(codepoint);
        const bool scripts_match = is_number == last_was_number &&
                (previous_script == script || script <= 2 || previous_script <= 2);

        const int index_of_current_codepoint = text[i].isLowSurrogate() ? i - 1 : i;

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

std::unique_ptr<KaraokeData::Line> SyllabifyBasic(const KaraokeData::Line& line)
{
    const QString line_text = line.GetText();
    std::unique_ptr<KaraokeData::ReadOnlyLine> new_line = std::make_unique<KaraokeData::ReadOnlyLine>();
    if (line_text.isEmpty())
        return std::move(new_line);

    const QVector<int> split_points = SyllabifyBasic(line_text);
    new_line->m_syllables.reserve(split_points.size() - 1);
    for (int i = 1; i < split_points.size(); ++i)
    {
        new_line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(
                line_text.mid(split_points[i - 1], split_points[i] - split_points[i - 1]),
                KaraokeData::PLACEHOLDER_TIME, KaraokeData::PLACEHOLDER_TIME));
    }

    return std::move(new_line);
}

}
