#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <string>

using boost::multiprecision::cpp_int;
using Byte = std::uint8_t;

struct RabinKeyPair {
    cpp_int p;
    cpp_int q;
    cpp_int n;
};

RabinKeyPair generateKeyPair(unsigned modulusBits);

bool savePublicKey(const std::string& fileName, const cpp_int& n);
bool savePrivateKey(const std::string& fileName, const cpp_int& p, const cpp_int& q);
bool loadPublicKey(const std::string& fileName, cpp_int& n);
bool loadPrivateKey(const std::string& fileName, cpp_int& p, cpp_int& q);

bool encryptFile(const std::string& inputFile,
                 const std::string& outputFile,
                 const std::string& publicKeyFile);

bool decryptFile(const std::string& inputFile,
                 const std::string& outputFile,
                 const std::string& privateKeyFile);
