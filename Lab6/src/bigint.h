#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class BigInt {
public:
    BigInt() = default;
    BigInt(uint64_t v);

    static BigInt fromHex(const std::string& hex);
    static BigInt fromBytesLE(const std::vector<uint8_t>& bytes);
    std::string toHex(size_t digits = 0) const;

    bool isZero() const { return d_.empty(); }
    size_t bitLength() const;
    bool bit(size_t i) const;

    friend int compare(const BigInt& a, const BigInt& b);
    friend bool operator==(const BigInt& a, const BigInt& b) {
        return a.d_ == b.d_;
    }
    friend bool operator<(const BigInt& a, const BigInt& b) {
        return compare(a, b) < 0;
    }

    friend BigInt operator+(const BigInt& a, const BigInt& b);
    friend BigInt operator-(const BigInt& a, const BigInt& b);
    friend BigInt operator*(const BigInt& a, const BigInt& b);
    friend BigInt operator%(const BigInt& a, const BigInt& m);

    static void divmod(const BigInt& a, const BigInt& b, BigInt& q,
                       BigInt& r);

private:
    void trim();
    std::vector<uint32_t> d_;
};

BigInt addMod(const BigInt& a, const BigInt& b, const BigInt& m);
BigInt subMod(const BigInt& a, const BigInt& b, const BigInt& m);
BigInt mulMod(const BigInt& a, const BigInt& b, const BigInt& m);
BigInt powMod(const BigInt& a, const BigInt& e, const BigInt& m);
BigInt invMod(const BigInt& a, const BigInt& p);
