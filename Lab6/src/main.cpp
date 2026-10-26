#include "gost3410.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gost;

namespace {

std::string defaultSigPath(const std::string& file) {
    const size_t slash = file.find_last_of('/');
    const size_t dot = file.find_last_of('.');
    if (dot != std::string::npos &&
        (slash == std::string::npos || dot > slash))
        return file.substr(0, dot) + ".sig";
    return file + ".sig";
}

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

int cmdSign(const std::vector<std::string>& args) {
    if (args.size() < 2 || args.size() > 3)
        throw std::invalid_argument("sign <private.key> <файл> [файл.sig]");
    const std::string sigPath =
        args.size() == 3 ? args[2] : defaultSigPath(args[1]);
    const PrivateKey key = PrivateKey::load(args[0]);
    const Signature sig = sign(key, readFile(args[1]));
    sig.save(sigPath, key.bits);
    std::printf("r: %s\n", sig.r.toHex(key.bits / 4).c_str());
    std::printf("s: %s\n", sig.s.toHex(key.bits / 4).c_str());
    std::printf("Подпись сохранена: %s\n", sigPath.c_str());
    return 0;
}

int cmdVerify(const std::vector<std::string>& args) {
    if (args.size() < 2 || args.size() > 3)
        throw std::invalid_argument(
            "verify <public.key> <файл> [файл.sig]");
    const std::string sigPath =
        args.size() == 3 ? args[2] : defaultSigPath(args[1]);
    const PublicKey key = PublicKey::load(args[0]);
    const bool ok =
        verify(key, readFile(args[1]), Signature::load(sigPath));
    std::printf("%s\n", ok ? "Подпись верна" : "Подпись неверна");
    return ok ? 0 : 1;
}

bool roundTrip(int bits, bool tamper) {
    PrivateKey priv;
    PublicKey pub;
    generateKeys(bits, priv, pub);
    const auto msg = bytes("Методы защиты информации");
    const Signature sig = sign(priv, msg);
    return verify(pub, tamper ? bytes("Методы защиты информации!") : msg,
                  sig) != tamper;
}

int cmdTest() {
    const Curve& c = curveExample();
    const BigInt d = BigInt::fromHex(
        "7A929ADE789BB9BE10ED359DD39A72C11B60961F49397EEE1D19CE9891EC3B28");
    const BigInt e = BigInt::fromHex(
        "2DFBC1B372D89A1188C09C52E0EEC61FCE52032AB1022E8E67ECE6672B043EE5");
    const BigInt k = BigInt::fromHex(
        "77105C9B20BCD3122823C8CF6FCC7B956DE33814E95B7FE64FED924594DCEAB3");
    const Point Q{BigInt::fromHex("7F2B49E270DB6D90D8595BEC458B50C5"
                                  "8585BA1D4E9B788F6689DBD8E56FD80B"),
                  BigInt::fromHex("26F1B489D6701DD185C8413A977B3CBB"
                                  "AF64D1C593D26627DFFB101A87FF77DA"),
                  false};
    const Signature expected{
        BigInt::fromHex("41AA28D2F1AB148280CD9ED56FEDA419"
                        "74053554A42767B83AD043FD39DC0493"),
        BigInt::fromHex("01456C64BA4642A1653C235A98A60249"
                        "BCD6D3F746B631DF928014F6C5BF9C40")};

    const Point Qd = c.multiply(d, c.P);
    Signature sig;
    signWithK(c, d, e, k, sig);
    Signature bad = expected;
    bad.s = addMod(bad.s, 1, c.q);

    struct Case {
        const char* name;
        bool ok;
    };
    const Case cases[] = {
        {"GOST example Q = dP", Qd.x == Q.x && Qd.y == Q.y},
        {"GOST example sign", sig.r == expected.r && sig.s == expected.s},
        {"GOST example verify", verifyE(c, Q, e, expected)},
        {"GOST example bad s", !verifyE(c, Q, e, bad)},
        {"256 sign/verify", roundTrip(256, false)},
        {"256 changed text", roundTrip(256, true)},
        {"512 sign/verify", roundTrip(512, false)},
        {"512 changed text", roundTrip(512, true)},
    };

    int failed = 0;
    for (const Case& t : cases) {
        failed += !t.ok;
        std::printf("%-21s%s\n", t.name, t.ok ? "OK" : "FAIL");
    }
    std::printf("%s\n", failed == 0 ? "Все тесты пройдены" : "Есть ошибки");
    return failed == 0 ? 0 : 1;
}

void usage() {
    std::printf(
        "gost3410 keygen [-b 256|512] [public.key] [private.key]\n"
        "gost3410 sign <private.key> <файл> [файл.sig]\n"
        "gost3410 verify <public.key> <файл> [файл.sig]\n"
        "gost3410 test\n");
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
        if (cmd == "sign") return cmdSign(args);
        if (cmd == "verify") return cmdVerify(args);
        if (cmd == "test") return cmdTest();
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Ошибка: %s\n", e.what());
        return 2;
    }
}
