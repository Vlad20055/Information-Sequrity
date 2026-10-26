#pragma once

#include <cstdint>
#include <cstddef>

class Rng {
public:
    virtual ~Rng() = default;
    virtual uint64_t next64() = 0;
    uint64_t below(uint64_t bound);

    bool bit() { return next64() & 1; }
};


class SystemRng final : public Rng {
public:
    uint64_t next64() override;

private:
    void refill();
    uint64_t buf_[32];
    size_t   pos_ = 32;
};


class TestRng final : public Rng {
public:
    explicit TestRng(uint64_t seed);
    uint64_t next64() override;

private:
    uint64_t s_[4];
};
