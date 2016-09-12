// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#pragma once

#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

namespace KaraokeContainer
{

class Container
{
public:
    virtual std::vector<char> ReadLyricsFile() = 0;
};

std::unique_ptr<Container> Load(const std::string& path);

}
