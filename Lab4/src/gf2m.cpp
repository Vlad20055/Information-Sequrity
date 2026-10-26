#include "gf2m.h"

#include <stdexcept>
#include <string>

uint32_t GF2m::defaultPoly(int m) {
    static const uint32_t table[17] = {
        0, 0,
        0x7,      // m=2 : x^2 + x + 1
        0xB,      // m=3 : x^3 + x + 1
        0x13,     // m=4 : x^4 + x + 1
        0x25,     // m=5 : x^5 + x^2 + 1
        0x43,     // m=6 : x^6 + x + 1
        0x83,     // m=7 : x^7 + x + 1
        0x11D,    // m=8 : x^8 + x^4 + x^3 + x^2 + 1
        0x211,    // m=9 : x^9 + x^4 + 1
        0x409,    // m=10: x^10 + x^3 + 1
        0x805,    // m=11: x^11 + x^2 + 1
        0x1053,   // m=12: x^12 + x^6 + x^4 + x + 1
        0x201B,   // m=13: x^13 + x^4 + x^3 + x + 1
        0x4443,   // m=14: x^14 + x^10 + x^6 + x + 1
        0x8003,   // m=15: x^15 + x + 1
        0x1100B,  // m=16: x^16 + x^12 + x^3 + x + 1
    };
    return (m >= 2 && m <= 16) ? table[m] : 0;
}

GF2m::GF2m(int m, uint32_t poly) : m_(m) {
    if (m < 2 || m > 16)
        throw std::invalid_argument("GF2m: m должно быть в диапазоне [2, 16]");

    poly_ = poly ? poly : defaultPoly(m);
    if ((poly_ >> m) != 1)
        throw std::invalid_argument("GF2m: степень модуля должна быть равна m");

    q_ = 1u << m;
    exp_.assign(2 * (q_ - 1), 0);
    log_.assign(q_, 0);

    std::vector<bool> seen(q_, false);
    uint32_t a = 1;
    for (uint32_t i = 0; i < q_ - 1; ++i) {
        if (a == 0 || seen[a])
            throw std::invalid_argument("GF2m: модуль " + std::to_string(poly_) +
                                        " не является примитивным многочленом");
        seen[a] = true;
        exp_[i] = static_cast<Elem>(a);
        log_[a] = i;
        a <<= 1;
        if (a & q_) a ^= poly_;
    }
    if (a != 1)
        throw std::invalid_argument("GF2m: модуль не является примитивным многочленом");

    for (uint32_t i = q_ - 1; i < 2 * (q_ - 1); ++i)
        exp_[i] = exp_[i - (q_ - 1)];
}

uint32_t GF2m::log(Elem a) const {
    if (a == 0 || a >= q_) throw std::domain_error("GF2m::log: аргумент вне поля или равен 0");
    return log_[a];
}

GF2m::Elem GF2m::inv(Elem a) const {
    if (a == 0) throw std::domain_error("GF2m::inv: обращение нуля");
    return exp_[(order() - log_[a]) % order()];
}

GF2m::Elem GF2m::div(Elem a, Elem b) const {
    if (b == 0) throw std::domain_error("GF2m::div: деление на ноль");
    if (a == 0) return 0;
    return exp_[log_[a] + order() - log_[b]];
}

GF2m::Elem GF2m::pow(Elem a, int64_t e) const {
    if (a == 0) {
        if (e < 0) throw std::domain_error("GF2m::pow: 0 в отрицательной степени");
        return e == 0 ? 1 : 0;
    }
    const int64_t n = order();
    int64_t k = (static_cast<int64_t>(log_[a]) * (e % n)) % n;
    if (k < 0) k += n;
    return exp_[k];
}

GF2m::Elem GF2m::sqrt(Elem a) const {
    if (a == 0) return 0;
    uint32_t i = log_[a];
    uint32_t j = (i % 2 == 0) ? i / 2 : (i + order()) / 2;
    return exp_[j];
}
