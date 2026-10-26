#pragma once

#include "gf2m.h"
#include "random.h"

#include <string>
#include <vector>

class Poly {
public:
    using Elem = GF2m::Elem;

    explicit Poly(const GF2m& f);
    Poly(const GF2m& f, std::vector<Elem> coeffs);
    static Poly monomial(const GF2m& f, Elem c, int deg);
    static Poly x(const GF2m& f) { return monomial(f, 1, 1); }
    static Poly one(const GF2m& f) { return monomial(f, 1, 0); }

    const GF2m& field() const { return *f_; }
    int  degree() const { return static_cast<int>(c_.size()) - 1; }
    bool isZero() const { return c_.empty(); }
    bool isOne()  const { return c_.size() == 1 && c_[0] == 1; }
    Elem lead()   const { return c_.empty() ? 0 : c_.back(); }
    const std::vector<Elem>& coeffs() const { return c_; }

    Elem operator[](int i) const { return (i >= 0 && i < static_cast<int>(c_.size())) ? c_[i] : 0; }
    void set(int i, Elem v);

    Poly operator+(const Poly& b) const;
    Poly operator-(const Poly& b) const { return *this + b; }
    Poly operator*(const Poly& b) const;
    Poly operator/(const Poly& b) const;
    Poly operator%(const Poly& b) const;
    Poly& operator+=(const Poly& b) { return *this = *this + b; }
    bool operator==(const Poly& b) const { return c_ == b.c_; }

    Poly scale(Elem k) const;
    Poly shift(int k) const;
    Poly monic() const;
    Poly square() const;
    Elem eval(Elem x) const;

    static void divmod(const Poly& a, const Poly& b, Poly& q, Poly& r);

    std::string toString() const;

private:
    void trim();
    const GF2m* f_;
    std::vector<Elem> c_;
};

Poly gcd(Poly a, Poly b);

void extGcd(const Poly& a, const Poly& b, Poly& d, Poly& s, Poly& t);

Poly invMod(const Poly& a, const Poly& g);

Poly sqrMod(const Poly& a, const Poly& g);

Poly sqrtMod(const Poly& a, const Poly& g);

bool isIrreducible(const Poly& g);

Poly randomIrreducible(const GF2m& f, int t, Rng& rng);
