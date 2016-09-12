// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <string>
#include <vector>

#include "KaraokeContainer/Container.h"

namespace KaraokeContainer
{

class PlainContainer final : public Container
{
public:
    PlainContainer(const std::string& path);

    virtual std::vector<char> ReadLyricsFile() override;

    static void SaveLyricsFile(std::string path, const std::string& content);

private:
    std::string m_path;
};

}
