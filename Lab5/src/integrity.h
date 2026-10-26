#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace integrity {

inline const char* const kGost256 = "GOST34.11-2012-256";
inline const char* const kGost512 = "GOST34.11-2012-512";
inline const char* const kSha1 = "SHA-1";

struct Digests {
    std::vector<uint8_t> gost256;
    std::vector<uint8_t> gost512;
    std::vector<uint8_t> sha1;
};

Digests computeDigests(const std::vector<uint8_t>& data);

struct ChecksumFile {
    std::string fileName;
    uint64_t size = 0;
    std::string gost256, gost512, sha1;

    void save(const std::string& path) const;
    static ChecksumFile load(const std::string& path);
};

ChecksumFile makeChecksums(const std::string& fileName,
                           const std::vector<uint8_t>& data);

std::string toHex(const std::vector<uint8_t>& d);
std::string toStandardHex(const std::vector<uint8_t>& d);
std::vector<uint8_t> fromStandardHex(const std::string& hex);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}
