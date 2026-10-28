#include "elgamal.h"

#include "random.h"

#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>

namespace elgamal {

namespace {

using Fields = std::map<std::string, std::string>;

std::string readText(const std::string& path) {
    const auto data = readFile(path);
    return std::string(data.begin(), data.end());
}

void writeText(const std::string& path, const std::string& text) {
    writeFile(path, std::vector<uint8_t>(text.begin(), text.end()));
}

bool splitField(std::string line, std::string& key, std::string& value) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const size_t colon = line.find(": ");
    if (colon == std::string::npos) return false;
    key = line.substr(0, colon);
    value = line.substr(colon + 2);
    return true;
}

Fields readFields(const std::string& path) {
    std::istringstream in(readText(path));
    Fields f;
    std::string line, key, value;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        if (!splitField(line, key, value))
            throw std::runtime_error(path + ": неверная строка");
        f[key] = value;
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

[[noreturn]] void badCipher() {
    throw std::runtime_error("неверный ключ или повреждённый шифртекст");
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
    if (k.d.isZero() || !(k.d < curveFor(k.bits).q))
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

size_t blockBytes(const Curve& c) {
    const size_t bits = c.p.bitLength() - 1;
    return bits > 8 ? (bits - 8) / 8 : 0;
}

Point encodeBlock(const Curve& c, const std::vector<uint8_t>& block) {
    const BigInt m = BigInt::fromBytesLE(block);
    for (uint32_t j = 0; j < 256; ++j) {
        Point M;
        if (c.liftX(m * BigInt(256) + BigInt(j), M)) return M;
    }
    throw std::runtime_error("не удалось представить блок точкой кривой");
}

std::vector<uint8_t> decodeBlock(const Curve& c, const Point& M) {
    const size_t n = blockBytes(c);
    const BigInt m = M.x / BigInt(256);
    if (M.infinity || m.bitLength() > 8 * n) badCipher();
    return m.toBytesLE(n);
}

CipherBlock encryptPoint(const Curve& c, const Point& Q, const Point& M,
                         const BigInt& k) {
    return {c.multiply(k, c.P), c.add(M, c.multiply(k, Q))};
}

Point decryptPoint(const Curve& c, const BigInt& d, const CipherBlock& cb) {
    return c.subtract(cb.C2, c.multiply(d, cb.C1));
}

void generateKeys(int bits, PrivateKey& priv, PublicKey& pub) {
    const Curve& c = curveFor(bits);
    priv.bits = pub.bits = bits;
    priv.d = randomBelow(c.q);
    pub.Q = c.multiply(priv.d, c.P);
}

std::string encrypt(const PublicKey& key,
                    const std::vector<uint8_t>& data) {
    const Curve& c = curveFor(key.bits);
    const size_t n = blockBytes(c), w = size_t(key.bits) / 4;
    const size_t blocks = (data.size() + n - 1) / n;

    std::ostringstream out;
    out << "bits: " << key.bits << "\n";
    out << "size: " << data.size() << "\n";
    out << "blocks: " << blocks << "\n";
    for (size_t b = 0; b < blocks; ++b) {
        std::vector<uint8_t> block(n, 0);
        for (size_t i = 0; i < n && b * n + i < data.size(); ++i)
            block[i] = data[b * n + i];
        CipherBlock cb;
        do {
            cb = encryptPoint(c, key.Q, encodeBlock(c, block),
                              randomBelow(c.q));
        } while (cb.C2.infinity);
        out << cb.C1.x.toHex(w) << ' ' << cb.C1.y.toHex(w) << ' '
            << cb.C2.x.toHex(w) << ' ' << cb.C2.y.toHex(w) << "\n";
    }
    return out.str();
}

std::vector<uint8_t> decrypt(const PrivateKey& key,
                             const std::string& cipher) {
    const Curve& c = curveFor(key.bits);
    std::istringstream in(cipher);
    std::string line, k, v;
    size_t size = 0, blocks = 0;
    for (const char* name : {"bits", "size", "blocks"}) {
        if (!std::getline(in, line) || !splitField(line, k, v) || k != name)
            badCipher();
        if (k == "bits" && std::stoi(v) != key.bits)
            throw std::runtime_error(
                "длина ключа не совпадает с шифртекстом");
        if (k == "size") size = std::stoull(v);
        if (k == "blocks") blocks = std::stoull(v);
    }
    const size_t n = blockBytes(c);
    if (blocks != (size + n - 1) / n) badCipher();

    std::vector<uint8_t> out;
    for (size_t b = 0; b < blocks; ++b) {
        std::string x1, y1, x2, y2;
        if (!(in >> x1 >> y1 >> x2 >> y2)) badCipher();
        const CipherBlock cb{
            {BigInt::fromHex(x1), BigInt::fromHex(y1), false},
            {BigInt::fromHex(x2), BigInt::fromHex(y2), false}};
        if (!c.contains(cb.C1) || !c.contains(cb.C2)) badCipher();
        const auto part = decodeBlock(c, decryptPoint(c, key.d, cb));
        out.insert(out.end(), part.begin(), part.end());
    }
    out.resize(size);
    return out;
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
