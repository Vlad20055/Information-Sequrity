#include "bigint.h"

#include <bit>
#include <stdexcept>

BigInt::BigInt(uint64_t v) {
    while (v) {
        d_.push_back(uint32_t(v));
        v >>= 32;
    }
}

void BigInt::trim() {
    while (!d_.empty() && d_.back() == 0) d_.pop_back();
}

BigInt BigInt::fromHex(const std::string& hex) {
    BigInt r;
    size_t bits = 0;
    for (size_t i = hex.size(); i-- > 0;) {
        const char c = hex[i];
        uint32_t v;
        if (c >= '0' && c <= '9')
            v = c - '0';
        else if (c >= 'a' && c <= 'f')
            v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            v = c - 'A' + 10;
        else
            throw std::invalid_argument("неверное шестнадцатеричное число");
        if (bits % 32 == 0) r.d_.push_back(0);
        r.d_.back() |= v << (bits % 32);
        bits += 4;
    }
    r.trim();
    return r;
}

BigInt BigInt::fromBytesLE(const std::vector<uint8_t>& bytes) {
    BigInt r;
    r.d_.assign((bytes.size() + 3) / 4, 0);
    for (size_t i = 0; i < bytes.size(); ++i)
        r.d_[i / 4] |= uint32_t(bytes[i]) << (8 * (i % 4));
    r.trim();
    return r;
}

std::string BigInt::toHex(size_t digits) const {
    static const char* hexDigits = "0123456789abcdef";
    std::string s;
    for (size_t i = d_.size(); i-- > 0;)
        for (int sh = 28; sh >= 0; sh -= 4)
            s += hexDigits[d_[i] >> sh & 15];
    const size_t first = s.find_first_not_of('0');
    s = first == std::string::npos ? "0" : s.substr(first);
    if (s.size() < digits) s.insert(0, digits - s.size(), '0');
    return s;
}

size_t BigInt::bitLength() const {
    if (d_.empty()) return 0;
    return 32 * d_.size() - std::countl_zero(d_.back());
}

bool BigInt::bit(size_t i) const {
    return i / 32 < d_.size() && (d_[i / 32] >> (i % 32) & 1);
}

int compare(const BigInt& a, const BigInt& b) {
    if (a.d_.size() != b.d_.size())
        return a.d_.size() < b.d_.size() ? -1 : 1;
    for (size_t i = a.d_.size(); i-- > 0;)
        if (a.d_[i] != b.d_[i]) return a.d_[i] < b.d_[i] ? -1 : 1;
    return 0;
}

BigInt operator+(const BigInt& a, const BigInt& b) {
    const BigInt& x = a.d_.size() >= b.d_.size() ? a : b;
    const BigInt& y = a.d_.size() >= b.d_.size() ? b : a;
    BigInt r;
    r.d_.resize(x.d_.size() + 1);
    uint64_t carry = 0;
    for (size_t i = 0; i < x.d_.size(); ++i) {
        const uint64_t s =
            uint64_t(x.d_[i]) + (i < y.d_.size() ? y.d_[i] : 0) + carry;
        r.d_[i] = uint32_t(s);
        carry = s >> 32;
    }
    r.d_[x.d_.size()] = uint32_t(carry);
    r.trim();
    return r;
}

BigInt operator-(const BigInt& a, const BigInt& b) {
    if (compare(a, b) < 0) throw std::underflow_error("BigInt: a < b");
    BigInt r;
    r.d_.resize(a.d_.size());
    int64_t borrow = 0;
    for (size_t i = 0; i < a.d_.size(); ++i) {
        int64_t t = int64_t(a.d_[i]) - borrow -
                    (i < b.d_.size() ? int64_t(b.d_[i]) : 0);
        borrow = t < 0;
        r.d_[i] = uint32_t(t);
    }
    r.trim();
    return r;
}

BigInt operator*(const BigInt& a, const BigInt& b) {
    if (a.isZero() || b.isZero()) return {};
    BigInt r;
    r.d_.assign(a.d_.size() + b.d_.size(), 0);
    for (size_t i = 0; i < a.d_.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.d_.size(); ++j) {
            const uint64_t t =
                uint64_t(a.d_[i]) * b.d_[j] + r.d_[i + j] + carry;
            r.d_[i + j] = uint32_t(t);
            carry = t >> 32;
        }
        r.d_[i + b.d_.size()] = uint32_t(carry);
    }
    r.trim();
    return r;
}

void BigInt::divmod(const BigInt& a, const BigInt& b, BigInt& q,
                    BigInt& r) {
    if (b.isZero()) throw std::domain_error("BigInt: деление на ноль");
    if (compare(a, b) < 0) {
        q = BigInt();
        r = a;
        return;
    }
    const size_t n = b.d_.size(), m = a.d_.size();
    if (n == 1) {
        uint64_t rem = 0;
        q.d_.assign(m, 0);
        for (size_t i = m; i-- > 0;) {
            const uint64_t cur = rem << 32 | a.d_[i];
            q.d_[i] = uint32_t(cur / b.d_[0]);
            rem = cur % b.d_[0];
        }
        q.trim();
        r = BigInt(rem);
        return;
    }

    const int s = std::countl_zero(b.d_[n - 1]);
    auto shl = [s](uint32_t hi, uint32_t lo) {
        return s ? uint32_t(hi << s | lo >> (32 - s)) : hi;
    };
    std::vector<uint32_t> vn(n), un(m + 1);
    for (size_t i = n - 1; i > 0; --i) vn[i] = shl(b.d_[i], b.d_[i - 1]);
    vn[0] = b.d_[0] << s;
    un[m] = shl(0, a.d_[m - 1]);
    for (size_t i = m - 1; i > 0; --i) un[i] = shl(a.d_[i], a.d_[i - 1]);
    un[0] = a.d_[0] << s;

    const uint64_t base = uint64_t(1) << 32;
    q.d_.assign(m - n + 1, 0);
    for (size_t j = m - n + 1; j-- > 0;) {
        const uint64_t num = uint64_t(un[j + n]) << 32 | un[j + n - 1];
        uint64_t qhat = num / vn[n - 1];
        uint64_t rhat = num % vn[n - 1];
        while (qhat >= base ||
               qhat * vn[n - 2] > (rhat << 32 | un[j + n - 2])) {
            --qhat;
            rhat += vn[n - 1];
            if (rhat >= base) break;
        }

        uint64_t carry = 0;
        int64_t borrow = 0;
        for (size_t i = 0; i < n; ++i) {
            const uint64_t p = qhat * vn[i] + carry;
            carry = p >> 32;
            const int64_t t =
                int64_t(un[i + j]) - borrow - int64_t(p & 0xFFFFFFFF);
            un[i + j] = uint32_t(t);
            borrow = t < 0;
        }
        const int64_t t = int64_t(un[j + n]) - borrow - int64_t(carry);
        un[j + n] = uint32_t(t);

        q.d_[j] = uint32_t(qhat);
        if (t < 0) {
            --q.d_[j];
            uint64_t c = 0;
            for (size_t i = 0; i < n; ++i) {
                const uint64_t sum = uint64_t(un[i + j]) + vn[i] + c;
                un[i + j] = uint32_t(sum);
                c = sum >> 32;
            }
            un[j + n] = uint32_t(un[j + n] + c);
        }
    }

    r.d_.assign(n, 0);
    for (size_t i = 0; i < n; ++i)
        r.d_[i] = s ? uint32_t(un[i] >> s | un[i + 1] << (32 - s)) : un[i];
    q.trim();
    r.trim();
}

BigInt operator%(const BigInt& a, const BigInt& m) {
    BigInt q, r;
    BigInt::divmod(a, m, q, r);
    return r;
}

BigInt addMod(const BigInt& a, const BigInt& b, const BigInt& m) {
    BigInt r = a + b;
    if (!(r < m)) r = r - m;
    return r;
}

BigInt subMod(const BigInt& a, const BigInt& b, const BigInt& m) {
    return a < b ? a + (m - b) : a - b;
}

BigInt mulMod(const BigInt& a, const BigInt& b, const BigInt& m) {
    return (a * b) % m;
}

BigInt powMod(const BigInt& a, const BigInt& e, const BigInt& m) {
    BigInt r = BigInt(1) % m, base = a % m;
    for (size_t i = e.bitLength(); i-- > 0;) {
        r = mulMod(r, r, m);
        if (e.bit(i)) r = mulMod(r, base, m);
    }
    return r;
}

BigInt invMod(const BigInt& a, const BigInt& p) {
    if ((a % p).isZero()) throw std::domain_error("BigInt: нет обратного");
    return powMod(a, p - BigInt(2), p);
}
