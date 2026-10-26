#pragma once

#include "bigint.h"

#include <string>

struct Point {
    BigInt x, y;
    bool infinity = false;
};

struct Curve {
    std::string name;
    int bits;
    BigInt p, a, b, q;
    Point P;

    bool contains(const Point& pt) const;
    Point add(const Point& u, const Point& v) const;
    Point multiply(const BigInt& k, const Point& pt) const;
};

const Curve& curve256();
const Curve& curve512();
const Curve& curveExample();
const Curve& curveFor(int bits);
