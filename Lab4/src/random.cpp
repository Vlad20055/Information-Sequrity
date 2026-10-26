#include "random.h"

#include <cerrno>
#include <stdexcept>
#include <sys/random.h>

uint64_t Rng::below(uint64_t bound) {
    if (bound == 0) throw std::invalid_argument("Rng::below: bound == 0");
    const uint64_t limit = UINT64_MAX - (UINT64_MAX % bound + 1) % bound;
    uint64_t x;
    do { x = next64(); } while (x > limit);
    return x % bound;
}

void SystemRng::refill() {
    auto* p = reinterpret_cast<unsigned char*>(buf_);
    size_t need = sizeof(buf_);
    while (need > 0) {
        ssize_t got = getrandom(p, need, 0);
        if (got < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("SystemRng: ошибка getrandom()");
        }
        p += got;
        need -= static_cast<size_t>(got);
    }
    pos_ = 0;
}

uint64_t SystemRng::next64() {
    if (pos_ == 32) refill();
    return buf_[pos_++];
}

static uint64_t splitmix64(uint64_t& x) {
    uint64_t z = (x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

TestRng::TestRng(uint64_t seed) {
    for (auto& s : s_) s = splitmix64(seed);
}

uint64_t TestRng::next64() {
    const uint64_t result = rotl(s_[1] * 5, 7) * 9;
    const uint64_t t = s_[1] << 17;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = rotl(s_[3], 45);
    return result;
}
