#include "binmatrix.h"

#include <bit>
#include <numeric>
#include <stdexcept>
#include <utility>


BitVec::BitVec(size_t n) : n_(n), w_((n + 63) / 64, 0) {}

void BitVec::clearTail() {
    if (n_ % 64 != 0) w_.back() &= (uint64_t(1) << (n_ % 64)) - 1;
}

BitVec& BitVec::operator^=(const BitVec& b) {
    if (n_ != b.n_) throw std::invalid_argument("BitVec: XOR векторов разной длины");
    for (size_t i = 0; i < w_.size(); ++i) w_[i] ^= b.w_[i];
    return *this;
}

size_t BitVec::weight() const {
    size_t s = 0;
    for (uint64_t x : w_) s += std::popcount(x);
    return s;
}

bool BitVec::isZero() const {
    for (uint64_t x : w_) if (x) return false;
    return true;
}

bool BitVec::dot(const BitVec& b) const {
    if (n_ != b.n_) throw std::invalid_argument("BitVec: dot векторов разной длины");
    uint64_t acc = 0;
    for (size_t i = 0; i < w_.size(); ++i) acc ^= w_[i] & b.w_[i];
    return std::popcount(acc) & 1;
}

BitVec BitVec::permuted(const std::vector<uint32_t>& perm) const {
    if (perm.size() != n_) throw std::invalid_argument("BitVec::permuted: неверная длина перестановки");
    BitVec out(n_);
    for (size_t j = 0; j < n_; ++j)
        if (get(perm[j])) out.set(j, true);
    return out;
}

BitVec BitVec::unpermuted(const std::vector<uint32_t>& perm) const {
    if (perm.size() != n_) throw std::invalid_argument("BitVec::unpermuted: неверная длина перестановки");
    BitVec out(n_);
    for (size_t j = 0; j < n_; ++j)
        if (get(j)) out.set(perm[j], true);
    return out;
}

std::vector<uint8_t> BitVec::toBytes() const {
    std::vector<uint8_t> out((n_ + 7) / 8, 0);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<uint8_t>(w_[i / 8] >> (8 * (i % 8)));
    return out;
}

BitVec BitVec::fromBytes(const uint8_t* data, size_t nbits) {
    BitVec v(nbits);
    const size_t nbytes = (nbits + 7) / 8;
    for (size_t i = 0; i < nbytes; ++i)
        v.w_[i / 8] |= uint64_t(data[i]) << (8 * (i % 8));
    v.clearTail();
    return v;
}

BitVec BitVec::random(size_t n, Rng& rng) {
    BitVec v(n);
    for (auto& x : v.w_) x = rng.next64();
    v.clearTail();
    return v;
}

BitVec BitVec::randomOfWeight(size_t n, size_t w, Rng& rng) {
    if (w > n) throw std::invalid_argument("BitVec::randomOfWeight: w > n");
    std::vector<uint32_t> idx(n);
    std::iota(idx.begin(), idx.end(), 0u);
    BitVec v(n);
    for (size_t i = 0; i < w; ++i) {
        size_t j = i + rng.below(n - i);
        std::swap(idx[i], idx[j]);
        v.set(idx[i], true);
    }
    return v;
}

std::string BitVec::toString() const {
    std::string s(n_, '0');
    for (size_t i = 0; i < n_; ++i) if (get(i)) s[i] = '1';
    return s;
}

BitMatrix::BitMatrix(size_t rows, size_t cols) : cols_(cols), r_(rows, BitVec(cols)) {}

BitMatrix BitMatrix::identity(size_t n) {
    BitMatrix m(n, n);
    for (size_t i = 0; i < n; ++i) m.set(i, i, true);
    return m;
}

BitMatrix BitMatrix::random(size_t rows, size_t cols, Rng& rng) {
    BitMatrix m(0, cols);
    m.r_.reserve(rows);
    for (size_t i = 0; i < rows; ++i) m.r_.push_back(BitVec::random(cols, rng));
    return m;
}

BitVec BitMatrix::mulLeft(const BitVec& v) const {
    if (v.size() != rows()) throw std::invalid_argument("BitMatrix::mulLeft: несовпадение размеров");
    BitVec out(cols_);
    for (size_t i = 0; i < rows(); ++i)
        if (v.get(i)) out ^= r_[i];
    return out;
}

BitVec BitMatrix::mulRight(const BitVec& v) const {
    if (v.size() != cols_) throw std::invalid_argument("BitMatrix::mulRight: несовпадение размеров");
    BitVec out(rows());
    for (size_t i = 0; i < rows(); ++i)
        if (r_[i].dot(v)) out.set(i, true);
    return out;
}

BitMatrix BitMatrix::operator*(const BitMatrix& b) const {
    if (cols_ != b.rows()) throw std::invalid_argument("BitMatrix::*: несовпадение размеров");
    BitMatrix c(0, b.cols_);
    c.r_.reserve(rows());
    for (size_t i = 0; i < rows(); ++i) c.r_.push_back(b.mulLeft(r_[i]));
    return c;
}

BitMatrix BitMatrix::transpose() const {
    BitMatrix t(cols_, rows());
    for (size_t i = 0; i < rows(); ++i)
        for (size_t j = 0; j < cols_; ++j)
            if (get(i, j)) t.set(j, i, true);
    return t;
}

BitMatrix BitMatrix::permuteColumns(const std::vector<uint32_t>& perm) const {
    BitMatrix out(0, cols_);
    out.r_.reserve(rows());
    for (const auto& row : r_) out.r_.push_back(row.permuted(perm));
    return out;
}

std::vector<size_t> BitMatrix::rref() {
    std::vector<size_t> pivots;
    size_t pr = 0;
    for (size_t col = 0; col < cols_ && pr < rows(); ++col) {
        size_t sel = pr;
        while (sel < rows() && !get(sel, col)) ++sel;
        if (sel == rows()) continue;
        std::swap(r_[pr], r_[sel]);
        for (size_t i = 0; i < rows(); ++i)
            if (i != pr && get(i, col)) r_[i] ^= r_[pr];
        pivots.push_back(col);
        ++pr;
    }
    return pivots;
}

size_t BitMatrix::rank() const {
    BitMatrix copy = *this;
    return copy.rref().size();
}

bool BitMatrix::inverse(BitMatrix& out) const {
    const size_t n = rows();
    if (n != cols_) throw std::invalid_argument("BitMatrix::inverse: матрица не квадратная");
    BitMatrix aug(n, 2 * n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) if (get(i, j)) aug.set(i, j, true);
        aug.set(i, n + i, true);
    }
    for (size_t col = 0; col < n; ++col) {
        size_t sel = col;
        while (sel < n && !aug.get(sel, col)) ++sel;
        if (sel == n) return false;
        std::swap(aug.r_[col], aug.r_[sel]);
        for (size_t i = 0; i < n; ++i)
            if (i != col && aug.get(i, col)) aug.r_[i] ^= aug.r_[col];
    }
    out = BitMatrix(n, n);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            if (aug.get(i, n + j)) out.set(i, j, true);
    return true;
}

BitMatrix BitMatrix::kernel(std::vector<size_t>& freeCols) const {
    BitMatrix R = *this;
    std::vector<size_t> pivots = R.rref();

    std::vector<bool> isPivot(cols_, false);
    for (size_t p : pivots) isPivot[p] = true;
    freeCols.clear();
    for (size_t c = 0; c < cols_; ++c) if (!isPivot[c]) freeCols.push_back(c);

    BitMatrix K(freeCols.size(), cols_);
    for (size_t k = 0; k < freeCols.size(); ++k) {
        const size_t f = freeCols[k];
        K.set(k, f, true);
        for (size_t i = 0; i < pivots.size(); ++i)
            if (R.get(i, f)) K.set(k, pivots[i], true);
    }
    return K;
}

BitMatrix BitMatrix::randomInvertible(size_t n, Rng& rng, BitMatrix& inv) {
    for (;;) {
        BitMatrix m = random(n, n, rng);
        if (m.inverse(inv)) return m;
    }
}

std::string BitMatrix::toString() const {
    std::string s;
    for (const auto& row : r_) s += row.toString() + "\n";
    return s;
}

std::vector<uint32_t> randomPermutation(size_t n, Rng& rng) {
    std::vector<uint32_t> p(n);
    std::iota(p.begin(), p.end(), 0u);
    for (size_t i = n; i > 1; --i)
        std::swap(p[i - 1], p[rng.below(i)]);
    return p;
}
