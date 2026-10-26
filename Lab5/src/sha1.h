#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Sha1 {
public:
    Sha1();

    void update(const uint8_t* data, size_t len);
    void update(const std::vector<uint8_t>& data) {
        update(data.data(), data.size());
    }
    std::vector<uint8_t> digest();

    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data);

private:
    void processBlock(const uint8_t* block);

    std::array<uint32_t, 5> H_;
    std::array<uint8_t, 64> buf_{};
    size_t bufLen_ = 0;
    uint64_t totalBits_ = 0;
    bool done_ = false;
};
