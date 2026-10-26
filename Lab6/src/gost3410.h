#pragma once

#include "bigint.h"
#include "ec.h"

#include <cstdint>
#include <string>
#include <vector>

namespace gost {

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

struct Signature {
    BigInt r, s;

    void save(const std::string& path, int bits) const;
    static Signature load(const std::string& path);
};

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub);

BigInt messageToE(const std::vector<uint8_t>& msg, const Curve& c);
bool signWithK(const Curve& c, const BigInt& d, const BigInt& e,
               const BigInt& k, Signature& sig);
bool verifyE(const Curve& c, const Point& Q, const BigInt& e,
             const Signature& sig);

Signature sign(const PrivateKey& key, const std::vector<uint8_t>& msg);
bool verify(const PublicKey& key, const std::vector<uint8_t>& msg,
            const Signature& sig);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}
