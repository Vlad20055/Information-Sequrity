#include "goppa.h"

#include <stdexcept>

GoppaCode::GoppaCode(const GF2m& f, Poly g, std::vector<Elem> support)
    : f_(&f), g_(std::move(g)), L_(std::move(support)), sqrtX_(f) {
    if (&g_.field() != f_) throw std::invalid_argument("GoppaCode: g задан над другим полем");
    if (g_.degree() < 1) throw std::invalid_argument("GoppaCode: степень g должна быть >= 1");
    if (L_.size() > f.size()) throw std::invalid_argument("GoppaCode: n > 2^m");
    std::vector<bool> seen(f.size(), false);
    for (Elem a : L_) {
        if (a >= f.size() || seen[a]) throw std::invalid_argument("GoppaCode: элементы носителя должны быть различны");
        seen[a] = true;
        if (g_.eval(a) == 0) throw std::invalid_argument("GoppaCode: g(α) = 0 для элемента носителя");
    }
    build();
}

GoppaCode GoppaCode::generate(const GF2m& f, int t, size_t n, Rng& rng) {
    if (n > f.size()) throw std::invalid_argument("GoppaCode::generate: n > 2^m");
    Poly g = randomIrreducible(f, t, rng);
    auto perm = randomPermutation(f.size(), rng);
    std::vector<Elem> L(n);
    for (size_t i = 0; i < n; ++i) L[i] = static_cast<Elem>(perm[i]);
    return GoppaCode(f, std::move(g), std::move(L));
}

void GoppaCode::build() {
    const GF2m& f = *f_;
    const int m = f.m(), t = g_.degree();
    const size_t n = L_.size();

    H_ = BitMatrix(static_cast<size_t>(m) * t, n);
    for (size_t i = 0; i < n; ++i) {
        const Elem a = L_[i];
        Elem v = f.inv(g_.eval(a));
        for (int j = 0; j < t; ++j) {
            for (int b = 0; b < m; ++b)
                if (v >> b & 1) H_.set(static_cast<size_t>(j) * m + b, i, true);
            v = f.mul(v, a);
        }
    }

    G_ = H_.kernel(info_);

    invLin_.clear();
    invLin_.reserve(n);
    for (size_t i = 0; i < n; ++i)
        invLin_.push_back(invMod(Poly(f, {L_[i], 1}), g_));

    sqrtX_ = sqrtMod(Poly::x(f), g_);
}

Poly GoppaCode::sqrtModG(const Poly& a) const {
    const GF2m& f = *f_;
    const Poly r = a % g_;
    std::vector<Elem> even, odd;
    for (int i = 0; i <= r.degree(); ++i)
        (i % 2 == 0 ? even : odd).push_back(f.sqrt(r[i]));
    return (Poly(f, std::move(even)) + sqrtX_ * Poly(f, std::move(odd))) % g_;
}

BitVec GoppaCode::extractMessage(const BitVec& codeword) const {
    BitVec u(info_.size());
    for (size_t i = 0; i < info_.size(); ++i)
        if (codeword.get(info_[i])) u.set(i, true);
    return u;
}

Poly GoppaCode::syndrome(const BitVec& r) const {
    if (r.size() != n()) throw std::invalid_argument("GoppaCode::syndrome: неверная длина вектора");
    std::vector<Elem> acc(g_.degree(), 0);
    for (size_t i = 0; i < n(); ++i) {
        if (!r.get(i)) continue;
        const auto& c = invLin_[i].coeffs();
        for (size_t j = 0; j < c.size(); ++j) acc[j] ^= c[j];
    }
    return Poly(*f_, std::move(acc));
}

Poly GoppaCode::errorLocator(const Poly& S) const {
    const GF2m& f = *f_;
    const int t = g_.degree();
    const Poly x = Poly::x(f);

    if (S.isZero()) return Poly::one(f);

    const Poly T = invMod(S, g_);
    const Poly tau = sqrtModG(T + x);

    Poly rPrev = g_, r = tau;
    Poly bPrev(f), b = Poly::one(f);
    while (r.degree() > t / 2) {
        Poly q(f), rem(f);
        Poly::divmod(rPrev, r, q, rem);
        rPrev = std::move(r); r = std::move(rem);
        Poly bNext = bPrev + q * b;
        bPrev = std::move(b); b = std::move(bNext);
    }

    const Poly& a = r;
    return a.square() + x * b.square();
}

bool GoppaCode::decode(const BitVec& r, BitVec& codeword, BitVec& error) const {
    error = BitVec(n());
    const Poly sigma = errorLocator(syndrome(r));
    const int deg = sigma.degree();
    if (deg > t()) return false;

    int roots = 0;
    for (size_t i = 0; i < n(); ++i)
        if (sigma.eval(L_[i]) == 0) { error.set(i, true); ++roots; }

    if (roots != deg) return false;

    codeword = r ^ error;
    return H_.mulRight(codeword).isZero();
}
