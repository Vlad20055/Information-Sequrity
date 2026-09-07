#include "utils.h"
#include <fstream>

bool hexToBytes(const std::string& hex, Byte* output, size_t outputSize) {
    if (hex.size() != outputSize * 2) return false;

    auto hexValue = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    for (size_t i = 0; i < outputSize; ++i) {
        int hi = hexValue(hex[2 * i]);
        int lo = hexValue(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) return false;
        output[i] = static_cast<Byte>((hi << 4) | lo);
    }
    return true;
}

bool readFile(const std::string& fileName, std::vector<Byte>& data) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) return false;

    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size < 0) return false;

    file.seekg(0, std::ios::beg);
    data.resize(static_cast<size_t>(size));

    if (size > 0) {
        file.read(reinterpret_cast<char*>(data.data()), size);
    }
    return static_cast<bool>(file) || file.eof();
}

bool writeFile(const std::string& fileName, const std::vector<Byte>& data) {
    std::ofstream file(fileName, std::ios::binary);
    if (!file) return false;

    if (!data.empty()) {
        file.write(reinterpret_cast<const char*>(data.data()),
                   static_cast<std::streamsize>(data.size()));
    }
    return static_cast<bool>(file);
}
