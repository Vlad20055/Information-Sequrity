#include "file_utils.hpp"

#include <fstream>

bool readBinaryFile(const std::string& fileName, std::vector<Byte>& data)
{
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return false;
    }
    file.seekg(0, std::ios::beg);

    data.resize(static_cast<std::size_t>(size));
    if (!data.empty()) {
        file.read(reinterpret_cast<char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }

    return static_cast<bool>(file) || file.eof();
}

bool writeBinaryFile(const std::string& fileName, const std::vector<Byte>& data)
{
    std::ofstream file(fileName, std::ios::binary);
    if (!file) {
        return false;
    }

    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }

    return static_cast<bool>(file);
}
