// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <QChar>
#include <QString>
#include <QVector>

#include "TextTransform/HangulUtils.h"
#include "TextTransform/Syllabify.h"

namespace TextTransform
{

static bool IsCJKSyllableEnd(const QString& text, int i)
{
    if (IsHangulSyllableEnd(text, i))
        return true;

    if (text[i].isHighSurrogate())
        return false;

    const uint codepoint = (!text[i].isLowSurrogate()) ? text[i].unicode() :
                               ((i == 0 || !text[i - 1].isHighSurrogate()) ? 0xFFFD :
                                   QChar::surrogateToUcs4(text[i - 1], text[i]));

    return codepoint == 0x3005 || codepoint == 0x3031 ||
           codepoint == 0x3032 || codepoint == 0x3035 ||
           (codepoint >= 0x3041 && codepoint <= 0x3096) ||
           (codepoint >= 0x309D && codepoint <= 0x309F) ||
           (codepoint >= 0x30A1 && codepoint <= 0x30FA) ||
           (codepoint >= 0x30FC && codepoint <= 0x30FF) ||
           (codepoint >= 0x31F0 && codepoint <= 0x31FF) ||
           (codepoint >= 0x3400 && codepoint <= 0x4DBF) ||
           (codepoint >= 0x4E00 && codepoint <= 0xA48F) ||
           (codepoint >= 0xF900 && codepoint <= 0xFAFF) ||
           (codepoint >= 0xFF66 && codepoint <= 0xFF9D) ||
           (codepoint >= 0x16FE0 && codepoint <= 0x16FE1) ||
           (codepoint >= 0x17000 && codepoint <= 0x187FF) ||
           (codepoint >= 0x1B000 && codepoint <= 0x1B12F) ||
           (codepoint >= 0x1B170 && codepoint <= 0x1B2FF) ||
           (codepoint >= 0x20000 && codepoint <= 0x2A6DF) ||
           (codepoint >= 0x2A700 && codepoint <= 0x2EBEF) ||
           (codepoint >= 0x2F800 && codepoint <= 0x2FA1F);
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

QVector<int> SyllabifyBasic(const QString& text)
{
    QVector<int> split_points;

    if (text.isEmpty())
        return split_points;

    split_points.append(0);

    for (int i = 0; i < text.size(); ++i)
    {
        const bool should_not_split_before = i != 0 &&
                 (text[i].isSpace() || text[i].isMark() || ModifiesPreviousKana(text[i]));
        if (should_not_split_before && split_points.last() == i)
            split_points.last() += 1;
        else if (text[i].isSpace() || IsCJKSyllableEnd(text, i))
            split_points.append(i + 1);
    }

    if (split_points.last() != text.size())
        split_points.append(text.size());

    return split_points;
}

}
