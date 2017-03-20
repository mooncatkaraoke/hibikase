// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <QChar>
#include <QString>

namespace TextTransform
{

bool IsHangulMedial(uint c);
bool IsHangulMedial(const QString& text, int i);
bool EndsWithHangulMedial(QChar c);
bool IsHangulFinal(uint c);
bool IsHangulFinal(const QString& text, int i);
bool EndsWithHangulFinal(QChar c);
bool IsHangulSyllableEnd(const QString& text, int i);

}
