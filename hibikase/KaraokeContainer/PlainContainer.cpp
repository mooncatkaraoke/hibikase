// You may choose to use this file under the terms of the CC0 license
// instead of or in addition to the license of Hibikase.
// This additional permission does not apply to Hibikase as a whole.
// http://creativecommons.org/publicdomain/zero/1.0/

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "KaraokeContainer/PlainContainer.h"

namespace KaraokeContainer
{

PlainContainer::PlainContainer(const std::string& path)
    : m_path(path)
{
}

std::vector<char> PlainContainer::ReadLyricsFile()
{
    // TODO: This doesn't work with non-ASCII paths on Windows
    std::ifstream file(m_path, std::ios::binary);
    // TODO: Should this be an exception?
    if (!file.is_open())
        return {};

    // TODO: This way of getting the file size is apparently not guaranteed to work
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0);

    std::vector<char> data;
    data.reserve(file_size);
    data.assign(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>());

    return data;
}

void PlainContainer::SaveLyricsFile(std::string path, const std::string& content)
{
    std::ofstream file(path, std::ios::binary);
    // TODO: Should this be an exception?
    if (!file.is_open())
        return;

    file.write(content.data(), content.size());
}

}
