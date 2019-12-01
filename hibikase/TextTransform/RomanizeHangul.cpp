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
#include <QHash>
#include <QString>
#include <QVector>

#include "KaraokeData/Song.h"
#include "KaraokeData/ReadOnlySong.h"
#include "TextTransform/HangulUtils.h"
#include "TextTransform/RomanizeHangul.h"

namespace TextTransform
{

static const QHash<QChar, QChar> FINAL_TO_INITIAL = {
    {QChar(u'ᆨ'), QChar(u'ᄀ')},
    {QChar(u'ᆩ'), QChar(u'ᄁ')},
    {QChar(u'ᆫ'), QChar(u'ᄂ')},
    {QChar(u'ᆮ'), QChar(u'ᄃ')},
    {QChar(u'ᆯ'), QChar(u'ᄅ')},
    {QChar(u'ᆷ'), QChar(u'ᄆ')},
    {QChar(u'ᆸ'), QChar(u'ᄇ')},
    {QChar(u'ᆺ'), QChar(u'ᄉ')},
    {QChar(u'ᆻ'), QChar(u'ᄊ')},
    {QChar(u'ᆼ'), QChar(u'ᄋ')},
    {QChar(u'ᆽ'), QChar(u'ᄌ')},
    {QChar(u'ᆾ'), QChar(u'ᄎ')},
    {QChar(u'ᆿ'), QChar(u'ᄏ')},
    {QChar(u'ᇀ'), QChar(u'ᄐ')},
    {QChar(u'ᇁ'), QChar(u'ᄑ')},
    {QChar(u'ᇂ'), QChar(u'ᄒ')},
};

static const QHash<QChar, QString> CLUSTER_DECOMPOSITIONS = {
    {QChar(u'ᆪ'), QStringLiteral(u"ᆨᆺ")},
    {QChar(u'ᆬ'), QStringLiteral(u"ᆫᆽ")},
    {QChar(u'ᆭ'), QStringLiteral(u"ᆫᇂ")},
    {QChar(u'ᆰ'), QStringLiteral(u"ᆯᆨ")},
    {QChar(u'ᆱ'), QStringLiteral(u"ᆯᆷ")},
    {QChar(u'ᆲ'), QStringLiteral(u"ᆯᆸ")},
    {QChar(u'ᆳ'), QStringLiteral(u"ᆯᆺ")},
    {QChar(u'ᆴ'), QStringLiteral(u"ᆯᇀ")},
    {QChar(u'ᆵ'), QStringLiteral(u"ᆯᇁ")},
    {QChar(u'ᆶ'), QStringLiteral(u"ᆯᇂ")},
    {QChar(u'ᆹ'), QStringLiteral(u"ᆸᆺ")},
};

static const QHash<QChar, QChar> FINAL_HOMOPHONES = {
    {QChar(u'ᆩ'), QChar(u'ᆨ')},
    {QChar(u'ᆺ'), QChar(u'ᆮ')},
    {QChar(u'ᆻ'), QChar(u'ᆮ')},
    {QChar(u'ᆽ'), QChar(u'ᆮ')},
    {QChar(u'ᆾ'), QChar(u'ᆮ')},
    {QChar(u'ᆿ'), QChar(u'ᆨ')},
    {QChar(u'ᇀ'), QChar(u'ᆮ')},
    {QChar(u'ᇁ'), QChar(u'ᆸ')},
    {QChar(u'ᇂ'), QChar(u'ᆮ')},
};

static const QHash<QChar, QString> INITIALS = {
    {QChar(u'ᄀ'), QStringLiteral("g")},
    {QChar(u'ᄁ'), QStringLiteral("kk")},
    {QChar(u'ᄂ'), QStringLiteral("n")},
    {QChar(u'ᄃ'), QStringLiteral("d")},
    {QChar(u'ᄄ'), QStringLiteral("tt")},
    {QChar(u'ᄅ'), QStringLiteral("r")},
    {QChar(u'ᄆ'), QStringLiteral("m")},
    {QChar(u'ᄇ'), QStringLiteral("b")},
    {QChar(u'ᄈ'), QStringLiteral("pp")},
    {QChar(u'ᄉ'), QStringLiteral("s")},
    {QChar(u'ᄊ'), QStringLiteral("ss")},
    {QChar(u'ᄋ'), QStringLiteral("")},
    {QChar(u'ᄌ'), QStringLiteral("j")},
    {QChar(u'ᄍ'), QStringLiteral("jj")},
    {QChar(u'ᄎ'), QStringLiteral("ch")},
    {QChar(u'ᄏ'), QStringLiteral("k")},
    {QChar(u'ᄐ'), QStringLiteral("t")},
    {QChar(u'ᄑ'), QStringLiteral("p")},
    {QChar(u'ᄒ'), QStringLiteral("h")},
};

static const QHash<QChar, QString> MEDIALS = {
    {QChar(u'ᅡ'), QStringLiteral("a")},
    {QChar(u'ᅢ'), QStringLiteral("ae")},
    {QChar(u'ᅣ'), QStringLiteral("ya")},
    {QChar(u'ᅤ'), QStringLiteral("yae")},
    {QChar(u'ᅥ'), QStringLiteral("eo")},
    {QChar(u'ᅦ'), QStringLiteral("e")},
    {QChar(u'ᅧ'), QStringLiteral("yeo")},
    {QChar(u'ᅨ'), QStringLiteral("ye")},
    {QChar(u'ᅩ'), QStringLiteral("o")},
    {QChar(u'ᅪ'), QStringLiteral("wa")},
    {QChar(u'ᅫ'), QStringLiteral("wae")},
    {QChar(u'ᅬ'), QStringLiteral("oe")},
    {QChar(u'ᅭ'), QStringLiteral("yo")},
    {QChar(u'ᅮ'), QStringLiteral("u")},
    {QChar(u'ᅯ'), QStringLiteral("wo")},
    {QChar(u'ᅰ'), QStringLiteral("we")},
    {QChar(u'ᅱ'), QStringLiteral("wi")},
    {QChar(u'ᅲ'), QStringLiteral("yu")},
    {QChar(u'ᅳ'), QStringLiteral("eu")},
    {QChar(u'ᅴ'), QStringLiteral("ui")},
    {QChar(u'ᅵ'), QStringLiteral("i")},
};

static const QHash<QChar, QString> FINALS = {
    {QChar(u'ᆨ'), QStringLiteral("k")},
    {QChar(u'ᆫ'), QStringLiteral("n")},
    {QChar(u'ᆮ'), QStringLiteral("t")},
    {QChar(u'ᆯ'), QStringLiteral("l")},
    {QChar(u'ᆷ'), QStringLiteral("m")},
    {QChar(u'ᆸ'), QStringLiteral("p")},
    {QChar(u'ᆼ'), QStringLiteral("ng")},
};

struct Syllable
{
    QString initials;  // 초성
    QString medials;   // 중성
    QString finals;    // 종성

    Syllable()
    {
    }

    Syllable(const QString& text)
    {
        for (QChar c : text)
            Add(c);
    }

    Syllable(const QStringRef& text)
    {
        for (QChar c : text)
            Add(c);
    }

    QString GetText() const
    {
        return initials + medials + finals;
    }

    bool IsEmpty() const
    {
        return initials.isEmpty() && medials.isEmpty() && finals.isEmpty();
    }

    void Add(QChar jamo)
    {
        uint c = jamo.unicode();
        if (IsHangulInitial(c))
        {
            if (c != 0x115F)
                initials += jamo;
        }
        else if (IsHangulMedial(c))
        {
            if (c != 0x1160)
                medials += jamo;
        }
        else if (IsHangulFinal(c))
        {
            finals += jamo;
        }
    }

    void Clear()
    {
        initials.clear();
        medials.clear();
        finals.clear();
    }
};

static bool operator==(const Syllable& lhs, const Syllable& rhs)
{
    return lhs.initials == rhs.initials && lhs.medials == rhs.medials && lhs.finals == rhs.finals;
}

static bool operator!=(const Syllable& lhs, const Syllable& rhs)
{
    return !(lhs == rhs);
}

static QString DecomposeHangul(QString text)
{
    QString result;
    result.reserve(text.size());
    for (QChar c : text)
    {
        if (IsPrecomposedHangulSyllable(c))
            result += QString(c).normalized(QString::NormalizationForm_D);
        else
            result += c;
    }
    return result;
}

// Returns the matching value from the lookup table if one exists.
// Otherwise, returns the passed-in value.
template <class A, class B>
static B Lookup(const QHash<A, B> lookup_table, A value)
{
    return lookup_table.value(value, value);
}

// The passed-in syllable must contain at least one initial
static void Palatalize(Syllable* syllable)
{
    static const QHash<QChar, QChar> PALATALIZATION = {
        {QChar(u'ᄃ'), QChar(u'ᄌ')},
        {QChar(u'ᄐ'), QChar(u'ᄎ')},
    };

    if (syllable->medials != QStringLiteral(u"ᅵ"))
        return;

    const int last_index = syllable->initials.size() - 1;
    const QChar c = syllable->initials[last_index];
    syllable->initials.replace(last_index, 1, Lookup(PALATALIZATION, c));
}

static void Resyllabify(QString* finals, Syllable* next_syllable)
{
    if (finals->isEmpty())
        return;

    const QChar last_final = (*finals)[finals->size() - 1];
    if (last_final == QChar(u'ᆼ') || next_syllable->initials != QStringLiteral(u"ᄋ"))
        return;

    finals->chop(1);

    if (last_final == QChar(u'ᇂ'))
        return Resyllabify(finals, next_syllable);

    next_syllable->initials = Lookup(FINAL_TO_INITIAL, last_final);
    Palatalize(next_syllable);
}

static void Aspirate(QString* finals, Syllable* next_syllable)
{
    static const QHash<QChar, QChar> ASPIRATION = {
        {QChar(u'ᄀ'), QChar(u'ᄏ')},
        {QChar(u'ᄃ'), QChar(u'ᄐ')},
        {QChar(u'ᄇ'), QChar(u'ᄑ')},
        {QChar(u'ᄉ'), QChar(u'ᄊ')},
        {QChar(u'ᄌ'), QChar(u'ᄎ')},
    };

    if (!finals->isEmpty() && !next_syllable->initials.isEmpty() &&
        next_syllable->initials[0] == QChar(u'ᄒ'))
    {
        const QChar last_final = (*finals)[finals->size() - 1];
        const QChar last_final_pronunciation = Lookup(FINAL_HOMOPHONES, last_final);
        auto it = ASPIRATION.constFind(Lookup(FINAL_TO_INITIAL, last_final_pronunciation));
        if (it != ASPIRATION.constEnd())
        {
            finals->chop(1);
            next_syllable->initials[0] = it.value();
            Palatalize(next_syllable);
        }
    }

    if (!finals->isEmpty() && !next_syllable->initials.isEmpty() &&
        (*finals)[finals->size() - 1] == QChar(u'ᇂ'))
    {
        const QChar next_initial = next_syllable->initials[0];
        auto it = ASPIRATION.constFind(next_initial);
        if (it != ASPIRATION.constEnd())
        {
            finals->chop(1);
            next_syllable->initials[0] = it.value();
        }
    }
}

static void ElideCluster(const Syllable& syllable, QString* finals, const Syllable& next_syllable)
{
    if (finals->size() != 2)
        return;

    const QChar next = !next_syllable.initials.isEmpty() ? next_syllable.initials[0] : QChar('\0');

    const bool next_is_k = next == QChar(u'ᄀ') || next == QChar(u'ᄁ') || next == QChar(u'ᄏ');
    const bool special_lk_case = !next_is_k && (*finals)[1] == QChar(u'ᆨ');

    const bool special_lp_case = syllable == QStringLiteral(u"밟") ||
            (syllable == QStringLiteral(u"넓") && (next_syllable == QStringLiteral(u"둥") ||
            next_syllable == QStringLiteral(u"죽") || next_syllable == QStringLiteral(u"적")));

    const bool remove_first = (*finals)[1] == QChar(u'ᇁ') || (*finals)[1] == QChar(u'ᆷ') ||
                              special_lk_case || special_lp_case;
    finals->remove(remove_first ? 0 : 1, 1);
}

static void AssimilateL(QCharRef final, QCharRef initial)
{
    if (final == QChar(u'ᆯ') && initial == QChar(u'ᄂ'))
        initial = QChar(u'ᄅ');
    else if (final == QChar(u'ᆫ') && initial == QChar(u'ᄅ'))
        final = QChar(u'ᆯ');
    else if (final != QChar(u'ᆯ') && initial == QChar(u'ᄅ'))
        initial = QChar(u'ᄂ');
}

static void AssimilateNasal(QCharRef final, QCharRef initial)
{
    static const QHash<QChar, QChar> NASALIZATION = {
        {QChar(u'ᆨ'), QChar(u'ᆼ')},
        {QChar(u'ᆮ'), QChar(u'ᆫ')},
        {QChar(u'ᆸ'), QChar(u'ᆷ')},
    };

    if (initial == QChar(u'ᄂ') || initial == QChar(u'ᄆ'))
    {
        auto it = NASALIZATION.constFind(Lookup(FINAL_HOMOPHONES, QChar(final)));
        if (it != NASALIZATION.constEnd())
            final = it.value();
    }
}

static QString RomanizeHangul(const Syllable& syllable, Syllable* next_syllable)
{
    QString result;

    for (QChar c : syllable.initials)
        result += Lookup(INITIALS, c);

    for (QChar c : syllable.medials)
        result += Lookup(MEDIALS, c);

    // The order of the transformations below is very important - be careful with changing it

    QString finals;
    for (QChar c : syllable.finals)
        finals += Lookup(CLUSTER_DECOMPOSITIONS, c);

    Resyllabify(&finals, next_syllable);
    Aspirate(&finals, next_syllable);
    ElideCluster(syllable, &finals, *next_syllable);
    if (!finals.isEmpty() && !next_syllable->initials.isEmpty())
    {
        QCharRef last_final = finals[finals.size() - 1];
        QCharRef first_initial = next_syllable->initials[0];
        AssimilateL(last_final, first_initial);
        AssimilateNasal(last_final, first_initial);
    }

    if (finals.right(1) == QStringLiteral(u"ᆯ") &&
        next_syllable->initials.left(1) == QStringLiteral(u"ᄅ"))
    {
        next_syllable->initials.replace(0, 1, 'l');
    }

    for (QChar c : finals)
        result += Lookup(FINALS, Lookup(FINAL_HOMOPHONES, c));

    return result;
}

// This function differs a little from IsHangulSyllableEnd in HangulUtils.cpp:
// 1. The input to this function must be decomposed, otherwise
//    precomposed hangul syllables will be treated as non-hangul characters.
// 2. This function will treat every non-hangul character as a whole syllable.
//    (RomanizeHangul(const QString&, QString) expects that no syllable
//    contains both hangul and non-hangul, but other than that, it doesn't
//    require non-hangul to be syllabified in any particular way.)
// 3. If a sequence of initials is not followed by a hangul character,
//    this function will treat that sequence of initials as a syllable.
static int FindHangulSyllableEnd(const QString& text, int i)
{
    if (!IsHangulJamo(text, i))
        return i + 1;

    while (i < text.size() && IsHangulInitial(text[i].unicode()))
        ++i;
    while (i < text.size() && IsHangulMedial(text[i].unicode()))
        ++i;
    while (i < text.size() && IsHangulFinal(text[i].unicode()))
        ++i;
    return i;
}

// All hangul in text must be decomposed.
// All hangul in next_text will be decomposed.
static QString RomanizeHangul(const QString& text, QString* next_text)
{
    QString result;
    result.reserve(text.size());

    Syllable prev_syllable;
    int i = 0;
    while (i < text.size())
    {
        const int syllable_end = FindHangulSyllableEnd(text, i);
        const bool is_hangul = IsHangulJamo(text[i]);
        Syllable syllable;

        if (is_hangul)
            syllable = Syllable(text.midRef(i, syllable_end - i));

        result += RomanizeHangul(prev_syllable, &syllable);
        prev_syllable = syllable;

        if (!is_hangul)
            result += text[i];

        i = syllable_end;
    }

    *next_text = DecomposeHangul(*next_text);
    Syllable next_syllable;
    int next_syllable_end = 0;
    if (IsHangulJamo(*next_text, 0))
    {
        next_syllable_end = FindHangulSyllableEnd(*next_text, 0);
        next_syllable = Syllable(next_text->leftRef(next_syllable_end));
    }

    result += RomanizeHangul(prev_syllable, &next_syllable);
    next_text->replace(0, next_syllable_end, next_syllable.GetText());

    return result;
}

std::unique_ptr<KaraokeData::Line> RomanizeHangul(
        const QVector<KaraokeData::Syllable*>& syllables, QString prefix)
{
    std::unique_ptr<KaraokeData::ReadOnlyLine> line = std::make_unique<KaraokeData::ReadOnlyLine>();
    line->m_syllables.reserve(syllables.size());

    QString empty;

    if (syllables.isEmpty())
    {
        line->m_prefix = RomanizeHangul(DecomposeHangul(prefix), &empty);
    }
    else
    {
        QString text = syllables[0]->GetText();
        line->m_prefix = RomanizeHangul(DecomposeHangul(prefix), &text);
        for (int i = 1; i < syllables.size(); ++i)
        {
            const KaraokeData::Syllable* syllable = syllables[i - 1];
            QString next_text = syllables[i]->GetText();
            line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(
                    RomanizeHangul(text, &next_text), syllable->GetStart(), syllable->GetEnd()));
            text = next_text;
        }
        const KaraokeData::Syllable* syllable = syllables[syllables.size() - 1];
        line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(
                    RomanizeHangul(text, &empty), syllable->GetStart(), syllable->GetEnd()));
    }

    return std::move(line);
}

}
