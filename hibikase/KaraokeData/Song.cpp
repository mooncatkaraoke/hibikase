// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <memory>

#include <QByteArray>
#include <QString>

#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiMoonCatSong.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

QString Line::GetText() const
{
    return m_text;
}

void Line::BuildText()
{
    // TODO: Performance cost of GetSyllables() copying pointers into a QVector?
    const QVector<Syllable*> syllables = GetSyllables();

    int size = GetPrefix().size() + GetSuffix().size();
    for (const Syllable* syllable : syllables)
        size += syllable->GetText().size();

    m_text.clear();
    m_text.reserve(size);
    m_text += GetPrefix();
    for (const Syllable* syllable : syllables)
        m_text += syllable->GetText();
    m_text += GetSuffix();
}

QString Song::GetText()
{
    // TODO: Performance cost of GetLines() copying pointers into a QVector?
    const QVector<Line*> lines = GetLines();

    QString text;
    for (Line* line : lines)
    {
        text += line->GetText();
        text += '\n';
    }
    return text;
}

std::unique_ptr<Song> Load(const QByteArray& data)
{
    std::unique_ptr<Song> vsqx = ParseVsqx(data);
    if (vsqx->IsValid())
        return vsqx;

    return std::make_unique<SoramimiMoonCatSong>(data);
}

}
