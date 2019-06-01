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
