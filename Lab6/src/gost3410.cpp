#include "gost3410.h"

#include "random.h"
#include "streebog.h"

#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>

namespace gost {

namespace {

using Fields = std::map<std::string, std::string>;

Fields readFields(const std::string& path) {
    const auto data = readFile(path);
    std::istringstream in(std::string(data.begin(), data.end()));
    Fields f;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const size_t colon = line.find(": ");
        if (colon == std::string::npos)
            throw std::runtime_error(path + ": неверная строка \"" + line +
                                     "\"");
        f[line.substr(0, colon)] = line.substr(colon + 2);
    }
    return f;
}

const std::string& field(const Fields& f, const std::string& key,
                         const std::string& path) {
    const auto it = f.find(key);
    if (it == f.end())
        throw std::runtime_error(path + ": нет поля \"" + key + "\"");
    return it->second;
}

void writeText(const std::string& path, const std::string& text) {
    writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
}

}

void PrivateKey::save(const std::string& path) const {
    std::ostringstream out;
    out << "bits: " << bits << "\n";
    out << "d: " << d.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

PrivateKey PrivateKey::load(const std::string& path) {
    const Fields f = readFields(path);
    PrivateKey k;
    k.bits = std::stoi(field(f, "bits", path));
    k.d = BigInt::fromHex(field(f, "d", path));
    const Curve& c = curveFor(k.bits);
    if (k.d.isZero() || !(k.d < c.q))
        throw std::runtime_error(path + ": недопустимый закрытый ключ");
    return k;
}

void PublicKey::save(const std::string& path) const {
    std::ostringstream out;
    out << "bits: " << bits << "\n";
    out << "Qx: " << Q.x.toHex(bits / 4) << "\n";
    out << "Qy: " << Q.y.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

PublicKey PublicKey::load(const std::string& path) {
    const Fields f = readFields(path);
    PublicKey k;
    k.bits = std::stoi(field(f, "bits", path));
    k.Q = {BigInt::fromHex(field(f, "Qx", path)),
           BigInt::fromHex(field(f, "Qy", path)), false};
    if (!curveFor(k.bits).contains(k.Q))
        throw std::runtime_error(path + ": точка Q не лежит на кривой");
    return k;
}

void Signature::save(const std::string& path, int bits) const {
    std::ostringstream out;
    out << "r: " << r.toHex(bits / 4) << "\n";
    out << "s: " << s.toHex(bits / 4) << "\n";
    writeText(path, out.str());
}

Signature Signature::load(const std::string& path) {
    const Fields f = readFields(path);
    return {BigInt::fromHex(field(f, "r", path)),
            BigInt::fromHex(field(f, "s", path))};
}

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub) {
    const Curve& c = curveFor(bits);
    priv.bits = pub.bits = bits;
    priv.d = randomBelow(c.q);
    pub.Q = c.multiply(priv.d, c.P);
}

BigInt messageToE(const std::vector<uint8_t>& msg, const Curve& c) {
    const BigInt alpha = BigInt::fromBytesLE(Streebog::hash(msg, c.bits));
    const BigInt e = alpha % c.q;
    return e.isZero() ? BigInt(1) : e;
}

bool signWithK(const Curve& c, const BigInt& d, const BigInt& e,
               const BigInt& k, Signature& sig) {
    const Point C = c.multiply(k, c.P);
    sig.r = C.x % c.q;
    if (sig.r.isZero()) return false;
    sig.s = addMod(mulMod(sig.r, d, c.q), mulMod(k, e, c.q), c.q);
    return !sig.s.isZero();
}

bool verifyE(const Curve& c, const Point& Q, const BigInt& e,
             const Signature& sig) {
    if (sig.r.isZero() || sig.s.isZero() || !(sig.r < c.q) ||
        !(sig.s < c.q))
        return false;
    const BigInt v = invMod(e, c.q);
    const BigInt z1 = mulMod(sig.s, v, c.q);
    const BigInt z2 = subMod(0, mulMod(sig.r, v, c.q), c.q);
    const Point C = c.add(c.multiply(z1, c.P), c.multiply(z2, Q));
    if (C.infinity) return false;
    return C.x % c.q == sig.r;
}

Signature sign(const PrivateKey& key, const std::vector<uint8_t>& msg) {
    const Curve& c = curveFor(key.bits);
    const BigInt e = messageToE(msg, c);
    Signature sig;
    while (!signWithK(c, key.d, e, randomBelow(c.q), sig)) {
    }
    return sig;
}

bool verify(const PublicKey& key, const std::vector<uint8_t>& msg,
            const Signature& sig) {
    const Curve& c = curveFor(key.bits);
    return verifyE(c, key.Q, messageToE(msg, c), sig);
}

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("не удаётся открыть файл: " + path);
    return {std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>()};
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("не удаётся создать файл: " + path);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out) throw std::runtime_error("ошибка записи в файл: " + path);
}

}
