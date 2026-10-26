// cmake -S . -B build
// cmake --build build -j

//   ./build/mceliece keygen  [public.key] [private.key] [-m M] [-t T] [-n N]
//   ./build/mceliece encrypt public.key examples/input.txt examples/secret.enc
//   ./build/mceliece decrypt private.key examples/secret.enc examples/output.txt

#include "mceliece.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

class Timer {
public:
    Timer() : start_(std::chrono::steady_clock::now()) {}
    double ms() const {
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_).count();
    }
private:
    std::chrono::steady_clock::time_point start_;
};

void header(const char* title) {
    std::printf("\n------------------------------------------------------------------\n");
    std::printf("  %s\n", title);
    std::printf("------------------------------------------------------------------\n");
}

std::string humanSize(uintmax_t bytes) {
    char buf[64];
    if (bytes < 1024) std::snprintf(buf, sizeof buf, "%ju Б", bytes);
    else std::snprintf(buf, sizeof buf, "%ju Б (%.1f КБ)", bytes, bytes / 1024.0);
    return buf;
}

std::string hex(const std::vector<uint8_t>& d, size_t from, size_t count) {
    std::string s;
    char b[4];
    for (size_t i = from; i < std::min(d.size(), from + count); ++i) {
        std::snprintf(b, sizeof b, "%02X ", d[i]);
        s += b;
    }
    if (from + count < d.size()) s += "...";
    return s;
}

std::string preview(const std::vector<uint8_t>& d, size_t maxBytes) {
    size_t end = std::min(d.size(), maxBytes);
    size_t lead = end;
    while (lead > 0 && end - lead < 4 && (d[lead - 1] & 0xC0) == 0x80) --lead;
    if (lead > 0 && d[lead - 1] >= 0xC0) {
        const uint8_t b = d[lead - 1];
        const size_t need = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : 2;
        if (end - (lead - 1) < need) end = lead - 1;
    }
    std::string s;
    for (size_t i = 0; i < end; ++i) {
        char c = static_cast<char>(d[i]);
        if (c == '\n') s += "\\n";
        else if (c == '\r') s += "\\r";
        else if (c == '\t') s += "\\t";
        else if (static_cast<unsigned char>(c) < 0x20) s += '.';
        else s += c;
    }
    if (end < d.size()) s += "...";
    return s;
}

void printBits(const char* label, const BitVec& v, size_t count = 64) {
    std::string s = v.toString().substr(0, std::min(count, v.size()));
    if (count < v.size()) s += "...";
    std::printf("  %-14s (%4zu бит): %s\n", label, v.size(), s.c_str());
}

void printCorner(const char* title, const BitMatrix& M, size_t r = 6, size_t c = 48) {
    std::printf("\n  %s (%zux%zu), левый верхний угол:\n", title, M.rows(), M.cols());
    for (size_t i = 0; i < std::min(r, M.rows()); ++i)
        std::printf("    %s ...\n", M.row(i).toString().substr(0, c).c_str());
    std::printf("    ...\n");
}

void printPositions(const BitVec& v) {
    size_t cnt = 0;
    for (size_t i = 0; i < v.size(); ++i)
        if (v.get(i)) std::printf("%s%4zu", cnt++ % 13 == 0 ? "\n    " : " ", i);
    std::printf("\n");
}

template <class T>
void printList(const std::vector<T>& v, size_t count = 16) {
    std::printf("    ");
    for (size_t i = 0; i < std::min(count, v.size()); ++i) std::printf("%u ", unsigned(v[i]));
    std::printf("%s\n", v.size() > count ? "..." : "");
}

void printParams(int m, size_t n, size_t k, int t) {
    std::printf("  Поле GF(2^%d), код Гоппы: n = %zu, k = %zu, t = %d\n", m, n, k, t);
    std::printf("  Блок открытого текста: %zu бит (%zu байт), блок шифртекста: %zu бит (%zu байт)\n",
                k, k / 8, n, (n + 7) / 8);
}

int cmdKeygen(const std::vector<std::string>& args) {
    mce::Params p;
    std::vector<std::string> files;
    bool mSet = false, nSet = false;
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto value = [&]() -> long {
            if (i + 1 >= args.size()) throw std::invalid_argument("после " + a + " нужно число");
            return std::stol(args[++i]);
        };
        if (a == "-m")      { p.m = int(value()); mSet = true; }
        else if (a == "-t") p.t = int(value());
        else if (a == "-n") { p.n = size_t(value()); nSet = true; }
        else files.push_back(a);
    }
    if (files.size() > 2) throw std::invalid_argument("слишком много аргументов");
    if (mSet && !nSet) p.n = size_t(1) << p.m;           // по умолчанию n = 2^m
    const std::string pubPath = files.size() > 0 ? files[0] : "public.key";
    const std::string privPath = files.size() > 1 ? files[1] : "private.key";

    // header("ГЕНЕРАЦИЯ КЛЮЧЕЙ МАК-ЭЛИСА");
    SystemRng rng;
    mce::PublicKey pub;
    mce::PrivateKey priv;
    Timer tm;
    mce::generateKeys(p, rng, pub, priv);
    const double genMs = tm.ms();
    pub.save(pubPath);
    priv.save(privPath);

    printParams(pub.m, pub.n, pub.k, pub.t);
    std::printf("  Время генерации: %.1f мс\n", genMs);

    const mce::Decryptor dec(priv);
    const GoppaCode& code = dec.code();

    // header("ЗАКРЫТЫЙ КЛЮЧ: (S^-1, g(x), L, P)");
    // std::printf("  Многочлен Гоппы g(x) степени %d (a — примитивный элемент GF(2^%d)):\n    %s\n",
    //             code.t(), pub.m, code.goppaPoly().toString().c_str());
    // std::printf("\n  Носитель L — элементы поля, метки позиций кода (%zu шт.):\n", priv.support.size());
    // printList(priv.support);
    // std::printf("\n  Перестановка P — столбец j матрицы G' берётся из столбца P[j] матрицы S*G:\n");
    // printList(priv.perm);
    // printCorner("Порождающая матрица кода Гоппы G", code.generator());
    // printCorner("Обратная невырожденная матрица S^-1", priv.Sinv);

    // header("ОТКРЫТЫЙ КЛЮЧ: (G' = S*G*P, t)");
    // printCorner("Матрица G'", pub.Gpub);

    // std::printf("\n  Открытый ключ: %s -> %s\n", pubPath.c_str(), humanSize(fs::file_size(pubPath)).c_str());
    // std::printf("  Закрытый ключ: %s -> %s\n", privPath.c_str(), humanSize(fs::file_size(privPath)).c_str());
    return 0;
}

int cmdEncrypt(const std::vector<std::string>& args) {
    if (args.size() != 3) throw std::invalid_argument("использование: encrypt <public.key> <вход> <выход>");
    header("ШИФРОВАНИЕ");
    const mce::PublicKey pub = mce::PublicKey::load(args[0]);
    const auto data = mce::readFile(args[1]);
    printParams(pub.m, pub.n, pub.k, pub.t);
    printCorner("Открытый ключ G'", pub.Gpub);

    SystemRng rng;
    mce::EncryptTrace tr;
    Timer tm;
    const auto ct = mce::encryptBytes(pub, data, rng, &tr);
    const double encMs = tm.ms();
    mce::writeFile(args[2], ct);

    // if (!data.empty()) {
    //     header("ПЕРВЫЙ БЛОК: c = m*G' + z");
    //     const size_t first = std::min(data.size(), pub.plainBlockBytes());
    //     std::printf("  Текст блока: %s\n",
    //                 preview(std::vector<uint8_t>(data.begin(), data.begin() + first), first).c_str());
    //     std::printf("\n");
    //     printBits("m", tr.msg);
    //     printBits("m*G'", pub.Gpub.mulLeft(tr.msg));
    //     printBits("z", tr.error);
    //     printBits("c = m*G' + z", tr.cipher);
    //     std::printf("\n  Вектор ошибок z: вес %zu, позиции:", tr.error.weight());
    //     printPositions(tr.error);
    // }

    // header("ИТОГ");
    // const size_t blocks = (data.size() + pub.plainBlockBytes() - 1) / pub.plainBlockBytes();
    // std::printf("  Исходный файл:      %s, %s\n", args[1].c_str(), humanSize(data.size()).c_str());
    // std::printf("  Текст:              %s\n", preview(data, 120).c_str());
    // std::printf("  Байты:              %s\n", hex(data, 0, 24).c_str());
    // std::printf("\n  Зашифрованный файл: %s, %s\n", args[2].c_str(), humanSize(ct.size()).c_str());
    // std::printf("  Байты (после заголовка): %s\n", hex(ct, 20, 24).c_str());
    // std::printf("\n  Блоков: %zu, коэффициент расширения: %.2f\n", blocks,
    //             data.empty() ? 0.0 : double(ct.size()) / data.size());
    std::printf("  Время шифрования: %.1f мс\n", encMs);
    return 0;
}

int cmdDecrypt(const std::vector<std::string>& args) {
    if (args.size() != 3) throw std::invalid_argument("использование: decrypt <private.key> <вход> <выход>");
    header("РАСШИФРОВАНИЕ");
    const mce::PrivateKey priv = mce::PrivateKey::load(args[0]);
    Timer tPrep;
    const mce::Decryptor dec(priv);
    const double prepMs = tPrep.ms();
    const auto ct = mce::readFile(args[1]);
    printParams(priv.m, priv.n, priv.k, priv.t);
    std::printf("  Многочлен Гоппы g(x):\n    %s\n", dec.code().goppaPoly().toString().c_str());

    mce::DecryptTrace tr;
    Timer tm;
    const auto data = mce::decryptBytes(dec, ct, &tr);
    const double decMs = tm.ms();
    mce::writeFile(args[2], data);

    // if (!data.empty()) {
    //     header("ПЕРВЫЙ БЛОК: c -> c*P^-1 -> Паттерсон -> m*S -> m");
    //     printBits("c", tr.cipher);
    //     printBits("c*P^-1", tr.permuted);
    //     std::printf("\n  Синдром S(x):                степень %d (ненулевой — ошибки есть)\n", tr.syndromeDeg);
    //     std::printf("  Многочлен локаторов sigma(x): степень %d\n", tr.sigmaDeg);
    //     std::printf("  Найдено и исправлено ошибок:  %zu, позиции (после P^-1):", tr.error.weight());
    //     printPositions(tr.error);
    //     std::printf("\n");
    //     printBits("m*S", tr.mS);
    //     printBits("m = (m*S)*S^-1", tr.msg);
    //     const size_t first = std::min(data.size(), priv.k / 8);
    //     std::printf("  Текст блока: %s\n",
    //                 preview(std::vector<uint8_t>(data.begin(), data.begin() + first), first).c_str());
    // }

    // header("ИТОГ");
    // std::printf("  Зашифрованный файл:  %s, %s\n", args[1].c_str(), humanSize(ct.size()).c_str());
    // std::printf("  Расшифрованный файл: %s, %s\n", args[2].c_str(), humanSize(data.size()).c_str());
    // std::printf("  Текст:               %s\n", preview(data, 120).c_str());
    // std::printf("\n  Восстановление кода Гоппы из ключа: %.1f мс\n", prepMs);
    std::printf("  Время расшифрования: %.1f мс\n", decMs);
    return 0;
}

int cmdCompare(const std::vector<std::string>& args) {
    if (args.size() != 2) throw std::invalid_argument("использование: compare <файл1> <файл2>");
    const auto a = mce::readFile(args[0]), b = mce::readFile(args[1]);
    std::printf("\n  %s: %s\n  %s: %s\n", args[0].c_str(), humanSize(a.size()).c_str(),
                args[1].c_str(), humanSize(b.size()).c_str());
    if (a == b) {
        std::printf("  Результат: файлы СОВПАДАЮТ побайтно\n");
        return 0;
    }
    size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i]) ++i;
    std::printf("  Результат: файлы РАЗЛИЧАЮТСЯ (первое отличие в байте %zu)\n", i);
    return 1;
}

void usage() {
    std::printf(
        "Криптосистема Мак-Элиса (двоичные коды Гоппы, декодер Паттерсона)\n\n"
        "  mceliece keygen  [public.key] [private.key] [-m M] [-t T] [-n N]\n"
        "                   по умолчанию m=10, t=38, n=1024 (k=644)\n"
        "  mceliece encrypt <public.key>  <входной файл> <зашифрованный файл>\n"
        "  mceliece decrypt <private.key> <зашифрованный файл> <выходной файл>\n"
        "  mceliece compare <файл1> <файл2>\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 1; }
    const std::string cmd = argv[1];
    const std::vector<std::string> args(argv + 2, argv + argc);
    try {
        if (cmd == "keygen")  return cmdKeygen(args);
        if (cmd == "encrypt") return cmdEncrypt(args);
        if (cmd == "decrypt") return cmdDecrypt(args);
        if (cmd == "compare") return cmdCompare(args);
        usage();
        return 1;
    } catch (const std::exception& e) {
        std::fflush(stdout);
        std::fprintf(stderr, "\nОшибка: %s\n", e.what());
        return 2;
    }
}