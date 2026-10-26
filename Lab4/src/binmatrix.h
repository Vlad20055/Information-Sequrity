#pragma once

#include "random.h"

#include <cstdint>
#include <string>
#include <vector>

class BitVec {
public:
    BitVec() = default;
    explicit BitVec(size_t n);

    size_t size()  const { return n_; }
    size_t words() const { return w_.size(); }

    bool get(size_t i) const { return (w_[i >> 6] >> (i & 63)) & 1; }
    void set(size_t i, bool v) {
        const uint64_t mask = uint64_t(1) << (i & 63);
        if (v) w_[i >> 6] |= mask; else w_[i >> 6] &= ~mask;
    }
    void flip(size_t i) { w_[i >> 6] ^= uint64_t(1) << (i & 63); }

    BitVec& operator^=(const BitVec& b);
    BitVec  operator^(const BitVec& b) const { BitVec r = *this; return r ^= b; }
    bool operator==(const BitVec& b) const { return n_ == b.n_ && w_ == b.w_; }
    bool operator!=(const BitVec& b) const { return !(*this == b); }

    size_t weight() const;
    bool   isZero() const;
    bool   dot(const BitVec& b) const;

    BitVec permuted(const std::vector<uint32_t>& perm) const;
    BitVec unpermuted(const std::vector<uint32_t>& perm) const;

    std::vector<uint8_t> toBytes() const;
    static BitVec fromBytes(const uint8_t* data, size_t nbits);

    static BitVec random(size_t n, Rng& rng);
    static BitVec randomOfWeight(size_t n, size_t w, Rng& rng);

    std::string toString() const;

    uint64_t*       data()       { return w_.data(); }
    const uint64_t* data() const { return w_.data(); }

private:
    void clearTail();
    size_t n_ = 0;
    std::vector<uint64_t> w_;
};

class BitMatrix {
public:
    BitMatrix() = default;
    BitMatrix(size_t rows, size_t cols);

    static BitMatrix identity(size_t n);
    static BitMatrix random(size_t rows, size_t cols, Rng& rng);

    size_t rows() const { return r_.size(); }
    size_t cols() const { return cols_; }

    bool get(size_t i, size_t j) const { return r_[i].get(j); }
    void set(size_t i, size_t j, bool v) { r_[i].set(j, v); }

    const BitVec& row(size_t i) const { return r_[i]; }
    BitVec&       row(size_t i)       { return r_[i]; }

    bool operator==(const BitMatrix& b) const { return cols_ == b.cols_ && r_ == b.r_; }

    BitMatrix operator*(const BitMatrix& b) const;
    BitVec    mulLeft(const BitVec& v) const;
    BitVec    mulRight(const BitVec& v) const;
    BitMatrix transpose() const;

    BitMatrix permuteColumns(const std::vector<uint32_t>& perm) const;

    std::vector<size_t> rref();
    size_t rank() const;

    bool inverse(BitMatrix& out) const;

    BitMatrix kernel(std::vector<size_t>& freeCols) const;

    static BitMatrix randomInvertible(size_t n, Rng& rng, BitMatrix& inv);

    std::string toString() const;

private:
    size_t cols_ = 0;
    std::vector<BitVec> r_;
};

std::vector<uint32_t> randomPermutation(size_t n, Rng& rng);
