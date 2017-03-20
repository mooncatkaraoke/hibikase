// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <QChar>
#include <QString>

#include "TextTransform/HangulUtils.h"

namespace TextTransform
{

// initial = 초성, medial = 중성, final = 종성

bool IsHangulMedial(uint c)
{
    return (c >= 0x1161 && c <= 0x11A7) || (c >= 0xD7B0 && c <= 0xD7C6);
}

bool IsHangulMedial(const QString& text, int i)
{
    return i < text.size() && IsHangulMedial(text[i].unicode());
}

bool EndsWithHangulMedial(QChar c)
{
    return IsHangulMedial(c.unicode()) ||
           (c.unicode() >= 0xAC00 && c.unicode() <= 0xD7A3 && c.unicode() % 28 == 0);
}

bool IsHangulFinal(uint c)
{
    return (c >= 0x11A8 && c <= 0x11FF) || (c >= 0xD7CB && c <= 0xD7FB);
}

bool IsHangulFinal(const QString& text, int i)
{
    return i < text.size() && IsHangulFinal(text[i].unicode());
}

bool EndsWithHangulFinal(QChar c)
{
    return IsHangulFinal(c.unicode()) ||
           (c.unicode() >= 0xAC00 && c.unicode() <= 0xD7A3 && c.unicode() % 28 != 0);
}

bool IsHangulSyllableEnd(const QString& text, int i)
{
    return (EndsWithHangulFinal(text[i]) && !IsHangulFinal(text, i + 1) ||
           (EndsWithHangulMedial(text[i]) && !IsHangulMedial(text, i + 1) &&
            !IsHangulFinal(text, i + 1)));
}

}
