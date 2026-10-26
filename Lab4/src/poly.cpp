#include "poly.h"

#include <algorithm>
#include <stdexcept>

// ======================= Базовые операции =======================

Poly::Poly(const GF2m& f) : f_(&f) {}

Poly::Poly(const GF2m& f, std::vector<Elem> coeffs) : f_(&f), c_(std::move(coeffs)) {
    for (Elem e : c_)
        if (e >= f.size()) throw std::invalid_argument("Poly: коэффициент вне поля");
    trim();
}

Poly Poly::monomial(const GF2m& f, Elem c, int deg) {
    Poly p(f);
    if (c != 0 && deg >= 0) {
        p.c_.assign(deg + 1, 0);
        p.c_[deg] = c;
    }
    return p;
}

void Poly::trim() {
    while (!c_.empty() && c_.back() == 0) c_.pop_back();
}

void Poly::set(int i, Elem v) {
    if (i < 0) throw std::out_of_range("Poly::set: отрицательная степень");
    if (i >= static_cast<int>(c_.size())) {
        if (v == 0) return;
        c_.resize(i + 1, 0);
    }
    c_[i] = v;
    trim();
}

Poly Poly::operator+(const Poly& b) const {
    const Poly& longer  = c_.size() >= b.c_.size() ? *this : b;
    const Poly& shorter = c_.size() >= b.c_.size() ? b : *this;
    Poly r = longer;
    for (size_t i = 0; i < shorter.c_.size(); ++i) r.c_[i] ^= shorter.c_[i];
    r.trim();
    return r;
}

Poly Poly::operator*(const Poly& b) const {
    Poly r(*f_);
    if (isZero() || b.isZero()) return r;
    r.c_.assign(c_.size() + b.c_.size() - 1, 0);
    for (size_t i = 0; i < c_.size(); ++i) {
        if (c_[i] == 0) continue;
        for (size_t j = 0; j < b.c_.size(); ++j)
            r.c_[i + j] ^= f_->mul(c_[i], b.c_[j]);
    }
    r.trim();
    return r;
}

void Poly::divmod(const Poly& a, const Poly& b, Poly& q, Poly& r) {
    if (b.isZero()) throw std::domain_error("Poly::divmod: деление на нулевой многочлен");
    const GF2m& f = *a.f_;
    q = Poly(f);
    r = a;
    const int db = b.degree();
    if (r.degree() < db) return;

    q.c_.assign(r.degree() - db + 1, 0);
    const Elem invLead = f.inv(b.lead());
    for (int i = r.degree(); i >= db; --i) {
        const Elem coef = f.mul(r.c_[i], invLead);
        if (coef == 0) continue;
        q.c_[i - db] = coef;
        for (int j = 0; j <= db; ++j)
            r.c_[i - db + j] ^= f.mul(coef, b.c_[j]);
    }
    q.trim();
    r.trim();
}

Poly Poly::operator/(const Poly& b) const { Poly q(*f_), r(*f_); divmod(*this, b, q, r); return q; }
Poly Poly::operator%(const Poly& b) const { Poly q(*f_), r(*f_); divmod(*this, b, q, r); return r; }

Poly Poly::scale(Elem k) const {
    if (k == 0) return Poly(*f_);
    Poly r = *this;
    for (Elem& e : r.c_) e = f_->mul(e, k);
    return r;
}

Poly Poly::shift(int k) const {
    if (isZero() || k == 0) return *this;
    if (k < 0) throw std::invalid_argument("Poly::shift: отрицательный сдвиг");
    Poly r = *this;
    r.c_.insert(r.c_.begin(), k, 0);
    return r;
}

Poly Poly::monic() const {
    if (isZero()) return *this;
    return scale(f_->inv(lead()));
}

Poly Poly::square() const {
    Poly r(*f_);
    if (isZero()) return r;
    r.c_.assign(2 * c_.size() - 1, 0);
    for (size_t i = 0; i < c_.size(); ++i) r.c_[2 * i] = f_->sqr(c_[i]);
    return r;
}

GF2m::Elem Poly::eval(Elem x) const {
    Elem acc = 0;
    for (int i = degree(); i >= 0; --i) acc = f_->mul(acc, x) ^ c_[i];
    return acc;
}

std::string Poly::toString() const {
    if (isZero()) return "0";
    std::string s;
    for (int i = degree(); i >= 0; --i) {
        if (c_[i] == 0) continue;
        if (!s.empty()) s += " + ";
        std::string coef;
        if (c_[i] != 1 || i == 0)
            coef = c_[i] == 1 ? "1" : "a^" + std::to_string(f_->log(c_[i]));
        std::string mono = i == 0 ? "" : (i == 1 ? "x" : "x^" + std::to_string(i));
        s += coef + (!coef.empty() && !mono.empty() ? " " : "") + mono;
    }
    return s;
}

// ======================= Алгоритмы =======================

Poly gcd(Poly a, Poly b) {
    while (!b.isZero()) {
        Poly r = a % b;
        a = std::move(b);
        b = std::move(r);
    }
    return a.monic();
}

void extGcd(const Poly& a, const Poly& b, Poly& d, Poly& s, Poly& t) {
    const GF2m& f = a.field();
    Poly r0 = a, r1 = b;
    Poly s0 = Poly::one(f), s1(f);
    Poly t0(f), t1 = Poly::one(f);
    while (!r1.isZero()) {
        Poly q(f), r(f);
        Poly::divmod(r0, r1, q, r);
        r0 = std::move(r1); r1 = std::move(r);
        Poly s2 = s0 - q * s1; s0 = std::move(s1); s1 = std::move(s2);
        Poly t2 = t0 - q * t1; t0 = std::move(t1); t1 = std::move(t2);
    }
    if (r0.isZero()) { d = r0; s = s0; t = t0; return; }
    const GF2m::Elem k = f.inv(r0.lead());
    d = r0.scale(k);
    s = s0.scale(k);
    t = t0.scale(k);
}

Poly invMod(const Poly& a, const Poly& g) {
    const GF2m& f = g.field();
    Poly d(f), s(f), t(f);
    extGcd(a % g, g, d, s, t);
    if (!d.isOne()) throw std::domain_error("invMod: многочлен необратим по модулю g");
    return s % g;
}

Poly sqrMod(const Poly& a, const Poly& g) {
    return a.square() % g;
}

Poly sqrtMod(const Poly& a, const Poly& g) {
    const int steps = g.field().m() * g.degree() - 1;
    Poly r = a % g;
    for (int i = 0; i < steps; ++i) r = sqrMod(r, g);
    return r;
}

bool isIrreducible(const Poly& g) {
    const int t = g.degree();
    if (t < 1) return false;
    if (t == 1) return true;
    const GF2m& f = g.field();
    const Poly x = Poly::x(f);
    Poly h = x % g;
    for (int i = 1; i <= t / 2; ++i) {
        for (int k = 0; k < f.m(); ++k) h = sqrMod(h, g);
        if (!gcd(g, h - x).isOne()) return false;
    }
    return true;
}

Poly randomIrreducible(const GF2m& f, int t, Rng& rng) {
    if (t < 1) throw std::invalid_argument("randomIrreducible: t должно быть >= 1");
    for (;;) {
        std::vector<GF2m::Elem> c(t + 1);
        for (int i = 0; i < t; ++i) c[i] = static_cast<GF2m::Elem>(rng.below(f.size()));
        c[t] = 1;
        Poly g(f, std::move(c));
        if (g[0] == 0) continue;
        if (isIrreducible(g)) return g;
    }
}