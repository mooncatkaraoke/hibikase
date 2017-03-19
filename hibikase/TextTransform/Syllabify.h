// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <QString>
#include <QVector>

namespace TextTransform
{

// Returns the syllable split points for a line of text
QVector<int> SyllabifyBasic(const QString& text);

}
