#pragma once

#include "bigint.h"
#include "ec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace elgamal {

struct PrivateKey {
    int bits = 256;
    BigInt d;

    void save(const std::string& path) const;
    static PrivateKey load(const std::string& path);
};

struct PublicKey {
    int bits = 256;
    Point Q;

    void save(const std::string& path) const;
    static PublicKey load(const std::string& path);
};

struct CipherBlock {
    Point C1, C2;
};

size_t blockBytes(const Curve& c);
Point encodeBlock(const Curve& c, const std::vector<uint8_t>& block);
std::vector<uint8_t> decodeBlock(const Curve& c, const Point& M);

CipherBlock encryptPoint(const Curve& c, const Point& Q, const Point& M,
                         const BigInt& k);
Point decryptPoint(const Curve& c, const BigInt& d, const CipherBlock& cb);

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub);
std::string encrypt(const PublicKey& key, const std::vector<uint8_t>& data);
std::vector<uint8_t> decrypt(const PrivateKey& key,
                             const std::string& cipher);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}
