/*
cmake -S . -B build && cmake --build build -j
./build/elgamal keygen examples/public.key examples/private.key     # -b 512 для 512 бит
./build/elgamal encrypt examples/public.key examples/input.txt examples/secret.enc
./build/elgamal decrypt examples/private.key examples/secret.enc examples/output.txt
./build/elgamal test
*/

#include "elgamal.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

using namespace elgamal;

namespace {

std::vector<uint8_t> bytes(const std::string& s) {
    return {s.begin(), s.end()};
}

int cmdKeygen(const std::vector<std::string>& args) {
    int bits = 256;
    std::vector<std::string> files;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "-b" && i + 1 < args.size())
            bits = std::stoi(args[++i]);
        else
            files.push_back(args[i]);
    }
    if (files.size() > 2)
        throw std::invalid_argument(
            "keygen [-b 256|512] [public] [private]");
    const std::string pubPath = files.size() > 0 ? files[0] : "public.key";
    const std::string privPath =
        files.size() > 1 ? files[1] : "private.key";

    PrivateKey priv;
    PublicKey pub;
    generateKeys(bits, priv, pub);
    pub.save(pubPath);
    priv.save(privPath);
    std::printf("Открытый ключ: %s\n", pubPath.c_str());
    std::printf("Закрытый ключ: %s\n", privPath.c_str());
    return 0;
}

int cmdEncrypt(const std::vector<std::string>& args) {
    if (args.size() != 3)
        throw std::invalid_argument("encrypt <public.key> <вход> <выход>");
    const PublicKey key = PublicKey::load(args[0]);
    const std::string cipher = encrypt(key, readFile(args[1]));
    writeFile(args[2], bytes(cipher));
    std::printf("Зашифровано: %s\n", args[2].c_str());
    return 0;
}

int cmdDecrypt(const std::vector<std::string>& args) {
    if (args.size() != 3)
        throw std::invalid_argument("decrypt <private.key> <вход> <выход>");
    const PrivateKey key = PrivateKey::load(args[0]);
    const auto data = readFile(args[1]);
    writeFile(args[2], decrypt(key, std::string(data.begin(), data.end())));
    std::printf("Расшифровано: %s\n", args[2].c_str());
    return 0;
}

bool roundTrip(int bits) {
    PrivateKey priv;
    PublicKey pub;
    generateKeys(bits, priv, pub);
    const auto msg = bytes(
        "Лабораторная работа №7. Аналог алгоритма Эль-Гамаля на "
        "эллиптических кривых. 0123456789");
    return decrypt(priv, encrypt(pub, msg)) == msg;
}

bool wrongKeyFails() {
    PrivateKey priv1, priv2;
    PublicKey pub1, pub2;
    generateKeys(256, priv1, pub1);
    generateKeys(256, priv2, pub2);
    const auto msg = bytes("Секретное сообщение для проверки ключа");
    try {
        return decrypt(priv2, encrypt(pub1, msg)) != msg;
    } catch (const std::exception&) {
        return true;
    }
}

bool randomized() {
    PrivateKey priv;
    PublicKey pub;
    generateKeys(256, priv, pub);
    const auto msg = bytes("Одинаковый текст");
    return encrypt(pub, msg) != encrypt(pub, msg);
}

bool encodeDecode() {
    const Curve& c = curve256();
    std::vector<uint8_t> block(blockBytes(c));
    for (size_t i = 0; i < block.size(); ++i) block[i] = uint8_t(7 * i + 1);
    const Point M = encodeBlock(c, block);
    return c.contains(M) && decodeBlock(c, M) == block;
}

int cmdTest() {
    const Curve& c = curveTextbook();
    const Point Pm{562, 201, false};
    const Point Pb{201, 5, false};
    const CipherBlock cb = encryptPoint(c, Pb, Pm, 386);
    const Point dec = decryptPoint(c, 58, cb);

    struct Case {
        const char* name;
        bool ok;
    };
    const Case cases[] = {
        {"Textbook C1 = kG", cb.C1.x == 676 && cb.C1.y == 558},
        {"Textbook C2 = Pm + kPb", cb.C2.x == 385 && cb.C2.y == 328},
        {"Textbook decrypt", dec.x == Pm.x && dec.y == Pm.y},
        {"Block <-> point", encodeDecode()},
        {"256 encrypt/decrypt", roundTrip(256)},
        {"512 encrypt/decrypt", roundTrip(512)},
        {"Random ciphertext", randomized()},
        {"Wrong key", wrongKeyFails()},
    };

    int failed = 0;
    for (const Case& t : cases) {
        failed += !t.ok;
        std::printf("%-24s%s\n", t.name, t.ok ? "OK" : "FAIL");
    }
    std::printf("%s\n", failed == 0 ? "Все тесты пройдены" : "Есть ошибки");
    return failed == 0 ? 0 : 1;
}

void usage() {
    std::printf(
        "elgamal keygen [-b 256|512] [public.key] [private.key]\n"
        "elgamal encrypt <public.key> <вход> <выход>\n"
        "elgamal decrypt <private.key> <вход> <выход>\n"
        "elgamal test\n");
}

}

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 1;
    }
    const std::string cmd = argv[1];
    const std::vector<std::string> args(argv + 2, argv + argc);
    try {
        if (cmd == "keygen") return cmdKeygen(args);
        if (cmd == "encrypt") return cmdEncrypt(args);
        if (cmd == "decrypt") return cmdDecrypt(args);
        if (cmd == "test") return cmdTest();
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Ошибка: %s\n", e.what());
        return 2;
    }
}
