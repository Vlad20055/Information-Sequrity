#pragma once

#include <cstdint>
#include <vector>

class GF2m {
public:
    using Elem = uint16_t;

    explicit GF2m(int m, uint32_t poly = 0);

    int      m()     const { return m_; }
    uint32_t poly()  const { return poly_; }
    uint32_t size()  const { return q_; }
    uint32_t order() const { return q_ - 1; }

    static Elem add(Elem a, Elem b) { return a ^ b; }

    Elem mul(Elem a, Elem b) const {
        if (a == 0 || b == 0) return 0;
        return exp_[log_[a] + log_[b]];
    }

    Elem sqr(Elem a) const { return mul(a, a); }

    Elem inv(Elem a) const;
    Elem div(Elem a, Elem b) const;
    Elem pow(Elem a, int64_t e) const;
    Elem sqrt(Elem a) const;

    Elem exp(uint32_t i) const { return exp_[i % order()]; }
    uint32_t log(Elem a) const;

    static uint32_t defaultPoly(int m);

private:
    int      m_;
    uint32_t poly_;
    uint32_t q_;
    std::vector<Elem>     exp_;
    std::vector<uint32_t> log_;
};
