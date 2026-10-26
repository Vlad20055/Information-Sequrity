/*
cmake -S . -B build && cmake --build build -j
./build/integrity hash   examples/input.txt          # или: hash -s "строка"
./build/integrity sign   examples/input.txt examples/input.hash
./build/integrity verify examples/input.txt examples/input.hash
./build/integrity test
*/

#include "integrity.h"
#include "sha1.h"
#include "streebog.h"

#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

using namespace integrity;

namespace {

std::string baseName(const std::string& path) {
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string defaultHashPath(const std::string& file) {
    const size_t slash = file.find_last_of('/');
    const size_t dot = file.find_last_of('.');
    if (dot != std::string::npos &&
        (slash == std::string::npos || dot > slash))
        return file.substr(0, dot) + ".hash";
    return file + ".hash";
}

void printLine(const char* name, const std::string& value) {
    std::printf("%-20s%s\n", (std::string(name) + ":").c_str(),
                value.c_str());
}

void printChecksums(const ChecksumFile& c) {
    printLine(kGost256, c.gost256);
    printLine(kGost512, c.gost512);
    printLine(kSha1, c.sha1);
}

int cmdHash(const std::vector<std::string>& args) {
    std::vector<uint8_t> data;
    if (args.size() == 2 && args[0] == "-s")
        data.assign(args[1].begin(), args[1].end());
    else if (args.size() == 1)
        data = readFile(args[0]);
    else
        throw std::invalid_argument("hash <файл> | hash -s <строка>");
    printChecksums(makeChecksums("", data));
    return 0;
}

int cmdSign(const std::vector<std::string>& args) {
    if (args.empty() || args.size() > 2)
        throw std::invalid_argument("sign <файл> [файл.hash]");
    const std::string hashPath =
        args.size() == 2 ? args[1] : defaultHashPath(args[0]);
    const ChecksumFile c =
        makeChecksums(baseName(args[0]), readFile(args[0]));
    c.save(hashPath);
    printChecksums(c);
    std::printf("Сохранено: %s\n", hashPath.c_str());
    return 0;
}

int cmdVerify(const std::vector<std::string>& args) {
    if (args.empty() || args.size() > 2)
        throw std::invalid_argument("verify <файл> [файл.hash]");
    const std::string hashPath =
        args.size() == 2 ? args[1] : defaultHashPath(args[0]);
    const ChecksumFile expected = ChecksumFile::load(hashPath);
    const ChecksumFile actual =
        makeChecksums(baseName(args[0]), readFile(args[0]));

    const bool ok256 = expected.gost256 == actual.gost256;
    const bool ok512 = expected.gost512 == actual.gost512;
    const bool okSha = expected.sha1 == actual.sha1;
    printLine(kGost256, ok256 ? "OK" : "НЕ СОВПАДАЕТ");
    printLine(kGost512, ok512 ? "OK" : "НЕ СОВПАДАЕТ");
    printLine(kSha1, okSha ? "OK" : "НЕ СОВПАДАЕТ");

    const bool ok = ok256 && ok512 && okSha;
    std::printf("%s\n", ok ? "Целостность подтверждена"
                           : "Целостность нарушена");
    return ok ? 0 : 1;
}

int cmdTest() {
    const std::string m1s =
        "012345678901234567890123456789012345678901234567890123456789012";
    const std::vector<uint8_t> m1(m1s.begin(), m1s.end());
    const std::vector<uint8_t> m2 = fromStandardHex(
        "fbe2e5f0eee3c820fbeafaebef20fffbf0e1e0f0f520e0ed20e8ece0ebe5f0f2"
        "f120fff0eeec20f120faf2fee5e2202ce8f6f3ede220e8e6eee1e8f0f2d1202c"
        "e8f0f2e5e220e5d1");
    auto bytes = [](const std::string& s) {
        return std::vector<uint8_t>(s.begin(), s.end());
    };

    struct Case {
        const char* name;
        std::string got;
        std::string expected;
    };
    const Case cases[] = {
        {"GOST-256 M1", toStandardHex(Streebog::hash(m1, 256)),
         "00557be5e584fd52a449b16b0251d05d27f94ab76cbaa6da890b59d8ef1e159d"},
        {"GOST-512 M1", toStandardHex(Streebog::hash(m1, 512)),
         "486f64c1917879417fef082b3381a4e211c324f074654c38823a7b76f830ad00"
         "fa1fbae42b1285c0352f227524bc9ab16254288dd6863dccd5b9f54a1ad0541b"},
        {"GOST-256 M2", toStandardHex(Streebog::hash(m2, 256)),
         "508f7e553c06501d749a66fc28c6cac0b005746d97537fa85d9e40904efed29d"},
        {"GOST-512 M2", toStandardHex(Streebog::hash(m2, 512)),
         "28fbc9bada033b1460642bdcddb90c3fb3e56c497ccd0f62b8a2ad4935e85f03"
         "7613966de4ee00531ae60f3b5a47f8dae06915d5f2f194996fcabf2622e6881e"},
        {"SHA-1 abc", toHex(Sha1::hash(bytes("abc"))),
         "a9993e364706816aba3e25717850c26c9cd0d89d"},
        {"SHA-1 empty", toHex(Sha1::hash({})),
         "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
        {"SHA-1 448 bit",
         toHex(Sha1::hash(bytes(
             "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"))),
         "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
        {"SHA-1 1M 'a'",
         toHex(Sha1::hash(std::vector<uint8_t>(1000000, 'a'))),
         "34aa973cd4c4daa4f61eeb2bdbad27316534016f"},
    };

    int failed = 0;
    for (const Case& c : cases) {
        const bool ok = c.got == c.expected;
        failed += !ok;
        std::printf("%-15s%s\n", c.name, ok ? "OK" : "FAIL");
    }
    std::printf("%s\n",
                failed == 0 ? "Все тесты пройдены" : "Есть ошибки");
    return failed == 0 ? 0 : 1;
}

void usage() {
    std::printf(
        "integrity hash <файл> | hash -s <строка>\n"
        "integrity sign <файл> [файл.hash]\n"
        "integrity verify <файл> [файл.hash]\n"
        "integrity test\n");
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
        if (cmd == "hash") return cmdHash(args);
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
