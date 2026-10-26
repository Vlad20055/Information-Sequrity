#include "integrity.h"

#include "sha1.h"
#include "streebog.h"

#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace integrity {

namespace {

bool isHex(const std::string& s, size_t len) {
    if (s.size() != len) return false;
    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    return true;
}

}

Digests computeDigests(const std::vector<uint8_t>& data) {
    return {Streebog::hash(data, 256), Streebog::hash(data, 512),
            Sha1::hash(data)};
}

ChecksumFile makeChecksums(const std::string& fileName,
                           const std::vector<uint8_t>& data) {
    const Digests d = computeDigests(data);
    ChecksumFile c;
    c.fileName = fileName;
    c.size = data.size();
    c.gost256 = toHex(d.gost256);
    c.gost512 = toHex(d.gost512);
    c.sha1 = toHex(d.sha1);
    return c;
}

void ChecksumFile::save(const std::string& path) const {
    std::ostringstream out;
    out << "file: " << fileName << "\n";
    out << "size: " << size << "\n";
    out << kGost256 << ": " << gost256 << "\n";
    out << kGost512 << ": " << gost512 << "\n";
    out << kSha1 << ": " << sha1 << "\n";
    const std::string s = out.str();
    writeFile(path, std::vector<uint8_t>(s.begin(), s.end()));
}

ChecksumFile ChecksumFile::load(const std::string& path) {
    const auto data = readFile(path);
    std::istringstream in(std::string(data.begin(), data.end()));
    ChecksumFile c;
    bool hasSize = false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const size_t colon = line.find(": ");
        if (colon == std::string::npos)
            throw std::runtime_error(
                "файл контрольных сумм: неверная строка \"" + line + "\"");
        const std::string key = line.substr(0, colon),
                          value = line.substr(colon + 2);
        if (key == "file") {
            c.fileName = value;
        } else if (key == "size") {
            c.size = std::stoull(value);
            hasSize = true;
        } else if (key == kGost256) {
            c.gost256 = value;
        } else if (key == kGost512) {
            c.gost512 = value;
        } else if (key == kSha1) {
            c.sha1 = value;
        } else {
            throw std::runtime_error(
                "файл контрольных сумм: неизвестное поле \"" + key + "\"");
        }
    }
    if (!hasSize || !isHex(c.gost256, 64) || !isHex(c.gost512, 128) ||
        !isHex(c.sha1, 40))
        throw std::runtime_error(
            "файл контрольных сумм повреждён или неполон: " + path);
    return c;
}

std::string toHex(const std::vector<uint8_t>& d) {
    static const char* digits = "0123456789abcdef";
    std::string s;
    s.reserve(2 * d.size());
    for (uint8_t b : d) {
        s += digits[b >> 4];
        s += digits[b & 15];
    }
    return s;
}

std::string toStandardHex(const std::vector<uint8_t>& d) {
    return toHex(std::vector<uint8_t>(d.rbegin(), d.rend()));
}

std::vector<uint8_t> fromStandardHex(const std::string& hex) {
    if (hex.size() % 2 != 0)
        throw std::invalid_argument(
            "нечётная длина шестнадцатеричной строки");
    std::vector<uint8_t> r;
    for (size_t i = hex.size(); i >= 2; i -= 2)
        r.push_back(uint8_t(std::stoi(hex.substr(i - 2, 2), nullptr, 16)));
    return r;
}

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("не удаётся открыть файл: " + path);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("не удаётся создать файл: " + path);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out) throw std::runtime_error("ошибка записи в файл: " + path);
}

}
