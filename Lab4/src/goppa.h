#pragma once

#include "binmatrix.h"
#include "gf2m.h"
#include "poly.h"
#include "random.h"

#include <vector>

class GoppaCode {
public:
    using Elem = GF2m::Elem;

    GoppaCode(const GF2m& f, Poly g, std::vector<Elem> support);
    static GoppaCode generate(const GF2m& f, int t, size_t n, Rng& rng);

    const GF2m& field() const { return *f_; }
    const Poly& goppaPoly() const { return g_; }
    const std::vector<Elem>& support() const { return L_; }
    size_t n() const { return L_.size(); }
    size_t k() const { return G_.rows(); }
    int    t() const { return g_.degree(); }

    const BitMatrix& parityCheck() const { return H_; }   // H: (m·t) × n
    const BitMatrix& generator()  const { return G_; }    // G: k × n, H·G.T = 0

    const std::vector<size_t>& infoSet() const { return info_; }

    BitVec encode(const BitVec& u) const { return G_.mulLeft(u); }
    BitVec extractMessage(const BitVec& codeword) const;

    Poly syndrome(const BitVec& r) const;
    bool decode(const BitVec& r, BitVec& codeword, BitVec& error) const;
    Poly errorLocator(const Poly& S) const;
    Poly sqrtModG(const Poly& a) const;

private:
    void build();

    const GF2m* f_;
    Poly g_;
    std::vector<Elem> L_;
    BitMatrix H_, G_;
    std::vector<size_t> info_;
    std::vector<Poly> invLin_;
    Poly sqrtX_;
};
