// SPDX-License-Identifier: GPL-2.0-or-later OR CC0-1.0

#pragma once

#include <QChar>
#include <QString>

namespace TextTransform
{

bool IsPrecomposedHangulSyllable(QChar c);
bool IsHangulInitial(uint c);
bool IsHangulInitial(const QString& text, int i);
bool IsHangulMedial(uint c);
bool IsHangulMedial(const QString& text, int i);
bool EndsWithHangulMedial(QChar c);
bool IsHangulFinal(uint c);
bool IsHangulFinal(const QString& text, int i);
bool EndsWithHangulFinal(QChar c);
bool IsHangulJamo(QChar c);
bool IsHangulJamo(const QString& text, int i);
bool IsHangul(QChar c);
bool IsHangulSyllableEnd(const QString& text, int i);

}
