#pragma once

#include "binmatrix.h"
#include "gf2m.h"
#include "goppa.h"
#include "random.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mce {

struct Params {
    int    m = 10;
    int    t = 38;
    size_t n = 1024;
};


struct EncryptTrace {
    BitVec msg;
    BitVec error;
    BitVec cipher;
};

struct DecryptTrace {
    BitVec cipher;
    BitVec permuted;
    int    syndromeDeg = -1;
    int    sigmaDeg = -1;
    BitVec error;
    BitVec mS;
    BitVec msg;
};


struct PublicKey {
    int    m = 0, t = 0;
    size_t n = 0, k = 0;
    BitMatrix Gpub;

    size_t plainBlockBytes() const { return k / 8; }
    size_t cipherBlockBytes() const { return (n + 7) / 8; }

    BitVec encryptBlock(const BitVec& msg, Rng& rng, EncryptTrace* trace = nullptr) const;

    void save(const std::string& path) const;
    static PublicKey load(const std::string& path);
};

struct PrivateKey {
    int      m = 0, t = 0;
    size_t   n = 0, k = 0;
    uint32_t fieldPoly = 0;
    std::vector<uint16_t> g;
    std::vector<uint16_t> support;
    std::vector<uint32_t> perm;
    BitMatrix Sinv;

    void save(const std::string& path) const;
    static PrivateKey load(const std::string& path);
};

void generateKeys(const Params& p, Rng& rng, PublicKey& pub, PrivateKey& priv);

class Decryptor {
public:
    explicit Decryptor(const PrivateKey& key);

    BitVec decryptBlock(const BitVec& c, DecryptTrace* trace = nullptr) const;

    const PrivateKey& key()  const { return key_; }
    const GoppaCode&  code() const { return *code_; }

private:
    PrivateKey key_;
    std::unique_ptr<GF2m> field_;
    std::unique_ptr<GoppaCode> code_;
};

std::vector<uint8_t> encryptBytes(const PublicKey& pub, const std::vector<uint8_t>& data, Rng& rng,
                                  EncryptTrace* firstBlock = nullptr);
std::vector<uint8_t> decryptBytes(const Decryptor& dec, const std::vector<uint8_t>& cipher,
                                  DecryptTrace* firstBlock = nullptr);

std::vector<uint8_t> readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

}  // namespace mce
