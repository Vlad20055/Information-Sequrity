#include "ec.h"

#include <stdexcept>

namespace {

struct Jacobian {
    BigInt X, Y, Z;
};

Jacobian toJacobian(const Point& pt) {
    if (pt.infinity) return {1, 1, 0};
    return {pt.x, pt.y, 1};
}

Point toAffine(const Curve& c, const Jacobian& J) {
    if (J.Z.isZero()) return {0, 0, true};
    const BigInt& p = c.p;
    const BigInt zi = invMod(J.Z, p);
    const BigInt zi2 = mulMod(zi, zi, p);
    return {mulMod(J.X, zi2, p), mulMod(J.Y, mulMod(zi2, zi, p), p), false};
}

Jacobian dbl(const Curve& c, const Jacobian& A) {
    if (A.Z.isZero() || A.Y.isZero()) return {1, 1, 0};
    const BigInt& p = c.p;
    const BigInt XX = mulMod(A.X, A.X, p);
    const BigInt YY = mulMod(A.Y, A.Y, p);
    const BigInt YYYY = mulMod(YY, YY, p);
    const BigInt ZZ = mulMod(A.Z, A.Z, p);
    const BigInt S = mulMod(4, mulMod(A.X, YY, p), p);
    const BigInt M =
        addMod(mulMod(3, XX, p), mulMod(c.a, mulMod(ZZ, ZZ, p), p), p);
    const BigInt X3 = subMod(mulMod(M, M, p), addMod(S, S, p), p);
    const BigInt Y3 =
        subMod(mulMod(M, subMod(S, X3, p), p), mulMod(8, YYYY, p), p);
    const BigInt Z3 = mulMod(addMod(A.Y, A.Y, p), A.Z, p);
    return {X3, Y3, Z3};
}

Jacobian add(const Curve& c, const Jacobian& A, const Jacobian& B) {
    if (A.Z.isZero()) return B;
    if (B.Z.isZero()) return A;
    const BigInt& p = c.p;
    const BigInt Z1Z1 = mulMod(A.Z, A.Z, p);
    const BigInt Z2Z2 = mulMod(B.Z, B.Z, p);
    const BigInt U1 = mulMod(A.X, Z2Z2, p);
    const BigInt U2 = mulMod(B.X, Z1Z1, p);
    const BigInt S1 = mulMod(A.Y, mulMod(B.Z, Z2Z2, p), p);
    const BigInt S2 = mulMod(B.Y, mulMod(A.Z, Z1Z1, p), p);
    if (U1 == U2) return S1 == S2 ? dbl(c, A) : Jacobian{1, 1, 0};
    const BigInt H = subMod(U2, U1, p);
    const BigInt R = subMod(S2, S1, p);
    const BigInt HH = mulMod(H, H, p);
    const BigInt HHH = mulMod(H, HH, p);
    const BigInt V = mulMod(U1, HH, p);
    const BigInt X3 =
        subMod(subMod(mulMod(R, R, p), HHH, p), addMod(V, V, p), p);
    const BigInt Y3 =
        subMod(mulMod(R, subMod(V, X3, p), p), mulMod(S1, HHH, p), p);
    const BigInt Z3 = mulMod(mulMod(A.Z, B.Z, p), H, p);
    return {X3, Y3, Z3};
}

Curve makeCurve(const char* name, int bits, const char* p, const char* a,
                const char* b, const char* q, const char* x,
                const char* y) {
    Curve c;
    c.name = name;
    c.bits = bits;
    c.p = BigInt::fromHex(p);
    c.a = BigInt::fromHex(a);
    c.b = BigInt::fromHex(b);
    c.q = BigInt::fromHex(q);
    c.P = {BigInt::fromHex(x), BigInt::fromHex(y), false};
    return c;
}

}

bool Curve::contains(const Point& pt) const {
    if (pt.infinity) return true;
    if (!(pt.x < p) || !(pt.y < p)) return false;
    const BigInt left = mulMod(pt.y, pt.y, p);
    const BigInt right =
        addMod(addMod(mulMod(mulMod(pt.x, pt.x, p), pt.x, p),
                      mulMod(a, pt.x, p), p),
               b, p);
    return left == right;
}

Point Curve::add(const Point& u, const Point& v) const {
    return toAffine(*this, ::add(*this, toJacobian(u), toJacobian(v)));
}

Point Curve::negate(const Point& pt) const {
    if (pt.infinity || pt.y.isZero()) return pt;
    return {pt.x, p - pt.y, false};
}

Point Curve::subtract(const Point& u, const Point& v) const {
    return add(u, negate(v));
}

bool Curve::liftX(const BigInt& x, Point& out) const {
    if (!(x < p)) return false;
    if (!(p % 4 == BigInt(3)))
        throw std::logic_error("требуется p = 3 (mod 4)");
    const BigInt rhs = addMod(
        addMod(mulMod(mulMod(x, x, p), x, p), mulMod(a, x, p), p), b, p);
    const BigInt y = powMod(rhs, (p + BigInt(1)) / BigInt(4), p);
    if (!(mulMod(y, y, p) == rhs)) return false;
    out = {x, y, false};
    return true;
}

Point Curve::multiply(const BigInt& k, const Point& pt) const {
    const Jacobian base = toJacobian(pt);
    Jacobian r{1, 1, 0};
    for (size_t i = k.bitLength(); i-- > 0;) {
        r = dbl(*this, r);
        if (k.bit(i)) r = ::add(*this, r, base);
    }
    return toAffine(*this, r);
}

const Curve& curve256() {
    static const Curve c = makeCurve(
        "id-GostR3410-2001-CryptoPro-A-ParamSet", 256,
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD97",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD94",
        "A6",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6C611070995AD10045841B09B761B893",
        "1",
        "8D91E471E0989CDA27DF505A453F2B7635294F2DDF23E3B122ACC99C9E9F1E14");
    return c;
}

const Curve& curve512() {
    static const Curve c = makeCurve(
        "id-tc26-gost-3410-12-512-paramSetA", 512,
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFDC7",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFDC4",
        "E8C2505DEDFC86DDC1BD0B2B6667F1DA34B82574761CB0E879BD081CFD0B6265"
        "EE3CB090F30D27614CB4574010DA90DD862EF9D4EBEE4761503190785A71C760",
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
        "27E69532F48D89116FF22B8D4E0560609B4B38ABFAD2B85DCACDB1411F10B275",
        "3",
        "7503CFE87A836AE3A61B8816E25450E6CE5E1C93ACF1ABC1778064FDCBEFA921"
        "DF1626BE4FD036E93D75E6A50E3A41E98028FE5FC235F5B889A589CB5215F2A4");
    return c;
}

const Curve& curveTextbook() {
    static const Curve c = makeCurve("E751(-1, 188)", 10, "2EF", "2EE",
                                     "BC", "2D7", "0", "178");
    return c;
}

const Curve& curveFor(int bits) {
    if (bits == 256) return curve256();
    if (bits == 512) return curve512();
    throw std::invalid_argument("длина ключа должна быть 256 или 512 бит");
}
