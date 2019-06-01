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

#include <QChar>
#include <QString>

#include "TextTransform/HangulUtils.h"

namespace TextTransform
{

// initial = 초성, medial = 중성, final = 종성

bool IsPrecomposedHangulSyllable(QChar c)
{
    return c.unicode() >= 0xAC00 && c.unicode() <= 0xD7A3;
}

bool IsHangulInitial(uint c)
{
    return (c >= 0x1100 && c <= 0x115F) || (c >= 0xA960 && c <= 0xA97C);
}

bool IsHangulInitial(const QString& text, int i)
{
    return i < text.size() ? IsHangulInitial(text[i].unicode()) : false;
}

bool IsHangulMedial(uint c)
{
    return (c >= 0x1160 && c <= 0x11A7) || (c >= 0xD7B0 && c <= 0xD7C6);
}

bool IsHangulMedial(const QString& text, int i)
{
    return i < text.size() && IsHangulMedial(text[i].unicode());
}

bool EndsWithHangulMedial(QChar c)
{
    return IsHangulMedial(c.unicode()) ||
           (IsPrecomposedHangulSyllable(c) && c.unicode() % 28 == 0);
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
           (IsPrecomposedHangulSyllable(c) && c.unicode() % 28 != 0);
}

bool IsHangulJamo(QChar c)
{
    return IsHangulInitial(c.unicode()) || IsHangulMedial(c.unicode()) ||
           IsHangulFinal(c.unicode());
}

bool IsHangulJamo(const QString& text, int i)
{
    return i < text.size() && IsHangulJamo(text[i].unicode());
}

bool IsHangul(QChar c)
{
    return IsPrecomposedHangulSyllable(c) || IsHangulJamo(c);
}

bool IsHangulSyllableEnd(const QString& text, int i)
{
    return (EndsWithHangulFinal(text[i]) && !IsHangulFinal(text, i + 1) ||
           (EndsWithHangulMedial(text[i]) && !IsHangulMedial(text, i + 1) &&
            !IsHangulFinal(text, i + 1)));
}

}
