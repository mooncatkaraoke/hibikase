// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <QChar>
#include <QHash>
#include <QPair>
#include <QString>
#include <QVector>

#include "KaraokeData/Song.h"
#include "TextTransform/HangulUtils.h"
#include "TextTransform/RomanizeHangul.h"

namespace TextTransform
{

// There seems to be an encoding problem with QStringLiteral on Windows...
// TODO: Is there a cleaner fix than using fromUtf8 as a workaround?
#if defined(_MSC_VER)
    #define UnicodeLiteral(a) QString::fromUtf8(a)
#else
    #define UnicodeLiteral(a) QStringLiteral(a)
#endif

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
    {QChar(u'ᆪ'), UnicodeLiteral("ᆨᆺ")},
    {QChar(u'ᆬ'), UnicodeLiteral("ᆫᆽ")},
    {QChar(u'ᆭ'), UnicodeLiteral("ᆫᇂ")},
    {QChar(u'ᆰ'), UnicodeLiteral("ᆯᆨ")},
    {QChar(u'ᆱ'), UnicodeLiteral("ᆯᆷ")},
    {QChar(u'ᆲ'), UnicodeLiteral("ᆯᆸ")},
    {QChar(u'ᆳ'), UnicodeLiteral("ᆯᆺ")},
    {QChar(u'ᆴ'), UnicodeLiteral("ᆯᇀ")},
    {QChar(u'ᆵ'), UnicodeLiteral("ᆯᇁ")},
    {QChar(u'ᆶ'), UnicodeLiteral("ᆯᇂ")},
    {QChar(u'ᆹ'), UnicodeLiteral("ᆸᆺ")},
};

static const QHash<QString, QString> CLUSTER_ELISION = {

    {UnicodeLiteral("ᆨᆺ"), UnicodeLiteral("ᆨ")},
    {UnicodeLiteral("ᆫᆽ"), UnicodeLiteral("ᆫ")},
    {UnicodeLiteral("ᆫᇂ"), UnicodeLiteral("ᆫ")},
    {UnicodeLiteral("ᆯᆨ"), UnicodeLiteral("ᆨ")},
    {UnicodeLiteral("ᆯᆷ"), UnicodeLiteral("ᆷ")},
    {UnicodeLiteral("ᆯᆸ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆯᆺ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆯᇀ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆯᇁ"), UnicodeLiteral("ᇁ")},
    {UnicodeLiteral("ᆯᇂ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆸᆺ"), UnicodeLiteral("ᆸ")},

    // The combinations below can occur when the last consonant of a
    // cluster changes due to assimilation with the next initial
    // consonant. The pronunciations are unconfirmed.
    {UnicodeLiteral("ᆨᆫ"), UnicodeLiteral("ᆼ")},
    {UnicodeLiteral("ᆫᆫ"), UnicodeLiteral("ᆫ")},
    {UnicodeLiteral("ᆯᆫ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆯᆼ"), UnicodeLiteral("ᆯ")},
    {UnicodeLiteral("ᆸᆫ"), UnicodeLiteral("ᆷ")},
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
};

static const QHash<QChar, QChar> PALATALIZATION = {
    {QChar(u'ᄃ'), QChar(u'ᄌ')},
    {QChar(u'ᄐ'), QChar(u'ᄎ')},
};

static const QHash<QPair<QChar, QChar>, QPair<QChar, QChar>> CONSONANT_ASSIMILATION = {
    {qMakePair(u'ᆨ', u'ᄂ'), qMakePair(u'ᆼ', u'ᄂ')},
    {qMakePair(u'ᆨ', u'ᄅ'), qMakePair(u'ᆼ', u'ᄂ')},
    {qMakePair(u'ᆨ', u'ᄆ'), qMakePair(u'ᆼ', u'ᄆ')},
    {qMakePair(u'ᆨ', u'ᄒ'), qMakePair('\0', u'ᄁ')},

    {qMakePair(u'ᆫ', u'ᄅ'), qMakePair(u'ᆯ', u'ᄅ')},

    {qMakePair(u'ᆮ', u'ᄂ'), qMakePair(u'ᆫ', u'ᄂ')},
    {qMakePair(u'ᆮ', u'ᄅ'), qMakePair(u'ᆫ', u'ᄂ')},
    {qMakePair(u'ᆮ', u'ᄆ'), qMakePair(u'ᆫ', u'ᄆ')},
    {qMakePair(u'ᆮ', u'ᄒ'), qMakePair('\0', u'ᄐ')},

    {qMakePair(u'ᆯ', u'ᄂ'), qMakePair(u'ᆯ', u'ᄅ')},

    {qMakePair(u'ᆷ', u'ᄅ'), qMakePair(u'ᆷ', u'ᄂ')},

    {qMakePair(u'ᆸ', u'ᄂ'), qMakePair(u'ᆷ', u'ᄂ')},
    {qMakePair(u'ᆸ', u'ᄅ'), qMakePair(u'ᆷ', u'ᄂ')},
    {qMakePair(u'ᆸ', u'ᄆ'), qMakePair(u'ᆷ', u'ᄆ')},
    {qMakePair(u'ᆸ', u'ᄒ'), qMakePair('\0', u'ᄑ')},

    {qMakePair(u'ᆼ', u'ᄅ'), qMakePair(u'ᆼ', u'ᄂ')},

    {qMakePair(u'ᇂ', u'ᄀ'), qMakePair('\0', u'ᄏ')},
    {qMakePair(u'ᇂ', u'ᄂ'), qMakePair(u'ᆫ', u'ᄂ')},
    {qMakePair(u'ᇂ', u'ᄃ'), qMakePair('\0', u'ᄐ')},
    {qMakePair(u'ᇂ', u'ᄅ'), qMakePair(u'ᆫ', u'ᄂ')},
    {qMakePair(u'ᇂ', u'ᄆ'), qMakePair(u'ᆫ', u'ᄆ')},
    {qMakePair(u'ᇂ', u'ᄇ'), qMakePair('\0', u'ᄑ')},
    {qMakePair(u'ᇂ', u'ᄉ'), qMakePair('\0', u'ᄊ')},
    {qMakePair(u'ᇂ', u'ᄌ'), qMakePair('\0', u'ᄎ')},
    {qMakePair(u'ᇂ', u'ᄒ'), qMakePair('\0', u'ᄐ')},
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
    {QChar(u'ᇂ'), QStringLiteral("t")},
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

static void AssimilateConstants(QString* finals, Syllable* next_syllable)
{
    if (finals->isEmpty() || next_syllable->initials.isEmpty())
        return;

    QChar next_initial = next_syllable->initials[0];
    bool i_affects_next_initial = false;

    {
        const QChar last_final = (*finals)[finals->size() - 1];
        if (last_final != QChar(u'ᆼ') && next_initial == QChar(u'ᄋ'))
        {
            next_initial = FINAL_TO_INITIAL.value(last_final, last_final);
            i_affects_next_initial = true;
            finals->chop(1);
        }
    }

    if (!finals->isEmpty())
    {
        i_affects_next_initial &= next_initial == QChar(u'ᄒ');

        QChar last_final = (*finals)[finals->size() - 1];
        QPair<QChar, QChar> consonants = qMakePair(last_final, next_initial);
        const QChar last_final_pronunciation = FINAL_HOMOPHONES.value(last_final, last_final);
        QPair<QChar, QChar> consonants_pronunciation = qMakePair(last_final_pronunciation,
                                                                 next_initial);
        consonants = CONSONANT_ASSIMILATION.value(consonants_pronunciation, consonants);
        last_final = consonants.first;
        next_initial = consonants.second;

        finals->chop(1);
        if (!last_final.isNull())
            finals->append(last_final);
    }

    if (i_affects_next_initial && next_syllable->medials == UnicodeLiteral("ᅵ"))
        next_initial = PALATALIZATION.value(next_initial, next_initial);

    next_syllable->initials.replace(0, 1, next_initial);
}

static QString RomanizeHangul(const Syllable& syllable, Syllable* next_syllable)
{
    QString result;

    for (QChar c : syllable.initials)
        result += INITIALS.value(c, QString(c));

    for (QChar c : syllable.medials)
        result += MEDIALS.value(c, QString(c));

    QString finals;
    for (QChar c : syllable.finals)
        finals += CLUSTER_DECOMPOSITIONS.value(c, QString(c));

    AssimilateConstants(&finals, next_syllable);

    finals = CLUSTER_ELISION.value(finals, finals);

    AssimilateConstants(&finals, next_syllable);

    if (finals.right(1) == UnicodeLiteral("ᆯ") &&
        next_syllable->initials.left(1) == UnicodeLiteral("ᄅ"))
    {
        next_syllable->initials.replace(0, 1, 'l');
    }

    for (QChar c : finals)
        result += FINALS.value(FINAL_HOMOPHONES.value(c, c), QString(c));

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

void RomanizeHangul(KaraokeData::Line* line)
{
    QVector<KaraokeData::Syllable*> syllables = line->GetSyllables();

    QString text;
    if (syllables.isEmpty())
    {
        text = line->GetSuffix();
        line->SetPrefix(RomanizeHangul(DecomposeHangul(line->GetPrefix()), &text));
    }
    else
    {
        text = syllables[0]->GetText();
        line->SetPrefix(RomanizeHangul(DecomposeHangul(line->GetPrefix()), &text));
        for (int i = 1; i < syllables.size(); ++i)
        {
            QString next_text = syllables[i]->GetText();
            syllables[i - 1]->SetText(RomanizeHangul(text, &next_text));
            text = next_text;
        }
        QString next_text = line->GetSuffix();
        syllables.last()->SetText(RomanizeHangul(text, &next_text));
        text = next_text;
    }
    line->SetSuffix(RomanizeHangul(text, &QString()));
}

}
