#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

class Streebog {
public:
    using Block = std::array<uint8_t, 64>;

    explicit Streebog(int bits = 512);

    void update(const uint8_t* data, size_t len);
    void update(const std::vector<uint8_t>& data) {
        update(data.data(), data.size());
    }
    std::vector<uint8_t> digest();

    int bits() const { return bits_; }

    static std::vector<uint8_t> hash(const std::vector<uint8_t>& data,
                                     int bits);

private:
    void compress(const Block& m, size_t lenBits);

    int bits_;
    Block h_{}, N_{}, Sigma_{};
    Block buf_{};
    size_t bufLen_ = 0;
    bool done_ = false;
};
