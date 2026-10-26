#include "sha1.h"

#include <algorithm>
#include <stdexcept>

namespace {

uint32_t rotl(uint32_t x, int s) {
    return x << s | x >> (32 - s);
}

uint32_t f(int t, uint32_t b, uint32_t c, uint32_t d) {
    if (t < 20) return (b & c) | (~b & d);
    if (t < 40) return b ^ c ^ d;
    if (t < 60) return (b & c) | (b & d) | (c & d);
    return b ^ c ^ d;
}

uint32_t K(int t) {
    if (t < 20) return 0x5A827999;
    if (t < 40) return 0x6ED9EBA1;
    if (t < 60) return 0x8F1BBCDC;
    return 0xCA62C1D6;
}

}

Sha1::Sha1()
    : H_{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0} {}

void Sha1::processBlock(const uint8_t* block) {
    uint32_t W[80];
    for (int t = 0; t < 16; ++t)
        W[t] = uint32_t(block[4 * t]) << 24 |
               uint32_t(block[4 * t + 1]) << 16 |
               uint32_t(block[4 * t + 2]) << 8 | uint32_t(block[4 * t + 3]);
    for (int t = 16; t < 80; ++t)
        W[t] = rotl(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);

    uint32_t a = H_[0], b = H_[1], c = H_[2], d = H_[3], e = H_[4];
    for (int t = 0; t < 80; ++t) {
        const uint32_t tmp = rotl(a, 5) + f(t, b, c, d) + e + W[t] + K(t);
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = tmp;
    }
    H_[0] += a;
    H_[1] += b;
    H_[2] += c;
    H_[3] += d;
    H_[4] += e;
}

void Sha1::update(const uint8_t* data, size_t len) {
    if (done_) throw std::logic_error("Sha1: хеш уже вычислен");
    totalBits_ += uint64_t(len) * 8;
    while (len > 0) {
        const size_t take = std::min(len, 64 - bufLen_);
        for (size_t i = 0; i < take; ++i) buf_[bufLen_ + i] = data[i];
        bufLen_ += take;
        data += take;
        len -= take;
        if (bufLen_ == 64) {
            processBlock(buf_.data());
            bufLen_ = 0;
        }
    }
}

std::vector<uint8_t> Sha1::digest() {
    if (done_) throw std::logic_error("Sha1: хеш уже вычислен");
    const uint64_t bits = totalBits_;

    const uint8_t one = 0x80, zero = 0x00;
    update(&one, 1);
    while (bufLen_ != 56) update(&zero, 1);

    uint8_t len[8];
    for (int i = 0; i < 8; ++i) len[i] = uint8_t(bits >> (56 - 8 * i));
    update(len, 8);
    done_ = true;

    std::vector<uint8_t> out(20);
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 4; ++j)
            out[4 * i + j] = uint8_t(H_[i] >> (24 - 8 * j));
    return out;
}

std::vector<uint8_t> Sha1::hash(const std::vector<uint8_t>& data) {
    Sha1 s;
    s.update(data);
    return s.digest();
}
