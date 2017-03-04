// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <memory>

#include <QByteArray>

#include "KaraokeData/Song.h"
#include "KaraokeData/SoramimiMoonCatSong.h"
#include "KaraokeData/VsqxParser.h"

namespace KaraokeData
{

std::unique_ptr<Song> Load(const QByteArray& data)
{
    std::unique_ptr<Song> vsqx = ParseVsqx(data);
    if (vsqx->IsValid())
        return vsqx;

    return std::make_unique<SoramimiMoonCatSong>(data);
}

}
