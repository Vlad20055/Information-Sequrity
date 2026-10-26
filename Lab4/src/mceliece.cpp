#include "mceliece.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace mce {

namespace {

class Writer {
public:
    void bytes(const void* p, size_t n) {
        auto* b = static_cast<const uint8_t*>(p);
        buf.insert(buf.end(), b, b + n);
    }
    void u16(uint16_t v) { for (int i = 0; i < 2; ++i) buf.push_back(uint8_t(v >> (8 * i))); }
    void u32(uint32_t v) { for (int i = 0; i < 4; ++i) buf.push_back(uint8_t(v >> (8 * i))); }
    void u64(uint64_t v) { for (int i = 0; i < 8; ++i) buf.push_back(uint8_t(v >> (8 * i))); }
    void bitvec(const BitVec& v) { auto b = v.toBytes(); bytes(b.data(), b.size()); }
    std::vector<uint8_t> buf;
};

class Reader {
public:
    Reader(const std::vector<uint8_t>& d, const char* what) : d_(d), what_(what) {}
    const uint8_t* take(size_t n) {
        if (n > d_.size() - pos_) fail("файл обрезан");
        const uint8_t* p = d_.data() + pos_;
        pos_ += n;
        return p;
    }
    uint64_t le(int n) { const uint8_t* p = take(n); uint64_t v = 0; for (int i = 0; i < n; ++i) v |= uint64_t(p[i]) << (8 * i); return v; }
    uint16_t u16() { return uint16_t(le(2)); }
    uint32_t u32() { return uint32_t(le(4)); }
    uint64_t u64() { return le(8); }
    BitVec bitvec(size_t nbits) { return BitVec::fromBytes(take((nbits + 7) / 8), nbits); }
    void magic(const char* m) { if (std::memcmp(take(4), m, 4) != 0) fail("неверная сигнатура"); }
    void end() const { if (pos_ != d_.size()) fail("лишние данные в конце"); }
    size_t remaining() const { return d_.size() - pos_; }
    [[noreturn]] void fail(const std::string& msg) const {
        throw std::runtime_error(std::string(what_) + ": " + msg);
    }
private:
    const std::vector<uint8_t>& d_;
    size_t pos_ = 0;
    const char* what_;
};

void checkParams(int m, int t, size_t n, size_t k, const Reader& r) {
    if (m < 2 || m > 16) r.fail("недопустимое m");
    if (n < 2 || n > (size_t(1) << m)) r.fail("недопустимое n");
    if (t < 1 || size_t(m) * t >= n) r.fail("недопустимое t");
    if (k == 0 || k > n) r.fail("недопустимое k");
}

}  // namespace

std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("не удаётся открыть файл: " + path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("не удаётся создать файл: " + path);
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) throw std::runtime_error("ошибка записи в файл: " + path);
}

void generateKeys(const Params& p, Rng& rng, PublicKey& pub, PrivateKey& priv) {
    GF2m field(p.m);
    if (p.n > field.size()) throw std::invalid_argument("generateKeys: n > 2^m");
    if (size_t(p.m) * p.t >= p.n) throw std::invalid_argument("generateKeys: m·t >= n, код пустой");

    GoppaCode code = GoppaCode::generate(field, p.t, p.n, rng);
    const size_t k = code.k();

    BitMatrix Sinv;
    BitMatrix S = BitMatrix::randomInvertible(k, rng, Sinv);

    std::vector<uint32_t> perm = randomPermutation(p.n, rng);

    pub.m = p.m; pub.t = p.t; pub.n = p.n; pub.k = k;
    pub.Gpub = (S * code.generator()).permuteColumns(perm);

    priv.m = p.m; priv.t = p.t; priv.n = p.n; priv.k = k;
    priv.fieldPoly = field.poly();
    priv.g.assign(code.goppaPoly().coeffs().begin(), code.goppaPoly().coeffs().end());
    priv.support.assign(code.support().begin(), code.support().end());
    priv.perm = std::move(perm);
    priv.Sinv = std::move(Sinv);
}

BitVec PublicKey::encryptBlock(const BitVec& msg, Rng& rng, EncryptTrace* trace) const {
    if (msg.size() != k) throw std::invalid_argument("encryptBlock: длина блока должна быть k");
    const BitVec z = BitVec::randomOfWeight(n, static_cast<size_t>(t), rng);
    BitVec c = Gpub.mulLeft(msg);                       // m · G'
    c ^= z;                                             // + z
    if (trace) *trace = {msg, z, c};
    return c;
}

Decryptor::Decryptor(const PrivateKey& key) : key_(key) {
    field_ = std::make_unique<GF2m>(key.m, key.fieldPoly);
    std::vector<GF2m::Elem> gc(key.g.begin(), key.g.end());
    std::vector<GF2m::Elem> L(key.support.begin(), key.support.end());
    code_ = std::make_unique<GoppaCode>(*field_, Poly(*field_, std::move(gc)), std::move(L));
    if (code_->k() != key.k || code_->t() != key.t)
        throw std::runtime_error("закрытый ключ повреждён: параметры кода не совпадают");
}

BitVec Decryptor::decryptBlock(const BitVec& c, DecryptTrace* trace) const {
    if (c.size() != key_.n) throw std::invalid_argument("decryptBlock: длина блока должна быть n");
    BitVec c1 = c.unpermuted(key_.perm);
    if (trace) {
        const Poly S = code_->syndrome(c1);
        trace->cipher = c;
        trace->permuted = c1;
        trace->syndromeDeg = S.degree();
        trace->sigmaDeg = code_->errorLocator(S).degree();
    }
    BitVec codeword, error;
    if (!code_->decode(c1, codeword, error))
        throw std::runtime_error("блок не декодируется: шифртекст повреждён или ключ не тот");
    BitVec mS = code_->extractMessage(codeword);
    BitVec m = key_.Sinv.mulLeft(mS);
    if (trace) {
        trace->error = error;
        trace->mS = mS;
        trace->msg = m;
    }
    return m;
}

void PublicKey::save(const std::string& path) const {
    Writer w;
    w.bytes("MCPK", 4);
    w.u32(m); w.u32(t); w.u32(uint32_t(n)); w.u32(uint32_t(k));
    for (size_t i = 0; i < k; ++i) w.bitvec(Gpub.row(i));
    writeFile(path, w.buf);
}

PublicKey PublicKey::load(const std::string& path) {
    auto data = readFile(path);
    Reader r(data, "открытый ключ");
    r.magic("MCPK");
    PublicKey p;
    p.m = int(r.u32()); p.t = int(r.u32()); p.n = r.u32(); p.k = r.u32();
    checkParams(p.m, p.t, p.n, p.k, r);
    p.Gpub = BitMatrix(p.k, p.n);
    for (size_t i = 0; i < p.k; ++i) p.Gpub.row(i) = r.bitvec(p.n);
    r.end();
    return p;
}

void PrivateKey::save(const std::string& path) const {
    Writer w;
    w.bytes("MCSK", 4);
    w.u32(m); w.u32(t); w.u32(uint32_t(n)); w.u32(uint32_t(k)); w.u32(fieldPoly);
    for (auto c : g) w.u16(c);
    for (auto a : support) w.u16(a);
    for (auto p : perm) w.u32(p);
    for (size_t i = 0; i < k; ++i) w.bitvec(Sinv.row(i));
    writeFile(path, w.buf);
}

PrivateKey PrivateKey::load(const std::string& path) {
    auto data = readFile(path);
    Reader r(data, "закрытый ключ");
    r.magic("MCSK");
    PrivateKey p;
    p.m = int(r.u32()); p.t = int(r.u32()); p.n = r.u32(); p.k = r.u32();
    p.fieldPoly = r.u32();
    checkParams(p.m, p.t, p.n, p.k, r);
    p.g.resize(p.t + 1);
    for (auto& c : p.g) c = r.u16();
    p.support.resize(p.n);
    for (auto& a : p.support) a = r.u16();
    p.perm.resize(p.n);
    std::vector<bool> seen(p.n, false);
    for (auto& x : p.perm) {
        x = r.u32();
        if (x >= p.n || seen[x]) r.fail("P не является перестановкой");
        seen[x] = true;
    }
    p.Sinv = BitMatrix(p.k, p.k);
    for (size_t i = 0; i < p.k; ++i) p.Sinv.row(i) = r.bitvec(p.k);
    r.end();
    return p;
}

std::vector<uint8_t> encryptBytes(const PublicKey& pub, const std::vector<uint8_t>& data, Rng& rng,
                                  EncryptTrace* firstBlock) {
    const size_t pb = pub.plainBlockBytes();
    if (pb == 0) throw std::invalid_argument("encryptBytes: k < 8");
    const size_t blocks = (data.size() + pb - 1) / pb;

    Writer w;
    w.bytes("MCE1", 4);
    w.u32(uint32_t(pub.n)); w.u32(uint32_t(pub.k)); w.u64(data.size());
    w.buf.reserve(w.buf.size() + blocks * pub.cipherBlockBytes());

    for (size_t b = 0; b < blocks; ++b) {
        const size_t off = b * pb;
        const size_t len = std::min(pb, data.size() - off);
        BitVec msg = BitVec::random(pub.k, rng);
        for (size_t i = 0; i < len; ++i)
            for (int bit = 0; bit < 8; ++bit)
                msg.set(8 * i + bit, data[off + i] >> bit & 1);
        w.bitvec(pub.encryptBlock(msg, rng, b == 0 ? firstBlock : nullptr));
    }
    return w.buf;
}

std::vector<uint8_t> decryptBytes(const Decryptor& dec, const std::vector<uint8_t>& cipher,
                                  DecryptTrace* firstBlock) {
    Reader r(cipher, "шифртекст");
    r.magic("MCE1");
    const size_t n = r.u32(), k = r.u32();
    const uint64_t len = r.u64();
    if (n != dec.key().n || k != dec.key().k)
        r.fail("параметры шифртекста не совпадают с ключом");
    const size_t pb = k / 8, cb = (n + 7) / 8;
    const uint64_t blocks = len / pb + (len % pb != 0);   // без переполнения при огромном len
    if (r.remaining() % cb != 0 || r.remaining() / cb != blocks)
        r.fail("неверное число блоков");

    std::vector<uint8_t> out;
    out.reserve(len);
    for (size_t b = 0; b < blocks; ++b) {
        BitVec msg = dec.decryptBlock(r.bitvec(n), b == 0 ? firstBlock : nullptr);
        const size_t take = std::min<uint64_t>(pb, len - out.size());
        auto bytes = msg.toBytes();
        out.insert(out.end(), bytes.begin(), bytes.begin() + take);
    }
    return out;
}

}  // namespace mce