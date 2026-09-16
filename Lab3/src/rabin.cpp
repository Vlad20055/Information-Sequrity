#include "rabin.hpp"
#include "file_utils.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {

constexpr std::size_t RECORD_SIZE = 31;
constexpr std::size_t MAX_PAYLOAD = 27;
constexpr char FILE_MAGIC[] = "RABIN1";

cpp_int modPositive(const cpp_int& value, const cpp_int& mod)
{
    cpp_int result = value % mod;
    if (result < 0) {
        result += mod;
    }
    return result;
}

cpp_int modPow(cpp_int base, cpp_int exponent, const cpp_int& mod)
{
    cpp_int result = 1;
    base = modPositive(base, mod);

    while (exponent > 0) {
        if ((exponent & 1) != 0) {
            result = (result * base) % mod;
        }
        base = (base * base) % mod;
        exponent >>= 1;
    }

    return result;
}

cpp_int extendedGcd(const cpp_int& a,
                    const cpp_int& b,
                    cpp_int& x,
                    cpp_int& y)
{
    cpp_int oldR = a;
    cpp_int r = b;
    cpp_int oldS = 1;
    cpp_int s = 0;
    cpp_int oldT = 0;
    cpp_int t = 1;

    while (r != 0) {
        const cpp_int quotient = oldR / r;

        cpp_int temp = oldR - quotient * r;
        oldR = r;
        r = temp;

        temp = oldS - quotient * s;
        oldS = s;
        s = temp;

        temp = oldT - quotient * t;
        oldT = t;
        t = temp;
    }

    x = oldS;
    y = oldT;
    return oldR;
}

cpp_int randomBits(unsigned bits, std::mt19937_64& rng)
{
    cpp_int value = 0;
    const unsigned fullWords = bits / 64;
    const unsigned remainingBits = bits % 64;

    for (unsigned i = 0; i < fullWords; ++i) {
        value <<= 64;
        value += static_cast<std::uint64_t>(rng());
    }

    if (remainingBits != 0) {
        value <<= remainingBits;
        std::uint64_t part = rng();
        if (remainingBits < 64) {
            part &= ((std::uint64_t(1) << remainingBits) - 1);
        }
        value += part;
    }

    value |= (cpp_int(1) << (bits - 1));
    value |= 1;
    return value;
}

bool isProbablePrime(const cpp_int& n,
                     unsigned rounds,
                     std::mt19937_64& rng)
{
    if (n < 2) {
        return false;
    }

    static const unsigned smallPrimes[] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
        31, 37, 41, 43, 47, 53, 59, 61, 67
    };

    for (unsigned p : smallPrimes) {
        if (n == p) {
            return true;
        }
        if (n % p == 0) {
            return false;
        }
    }

    cpp_int d = n - 1;
    unsigned s = 0;
    while ((d & 1) == 0) {
        d >>= 1;
        ++s;
    }

    static const unsigned fixedBases[] = {
        2, 3, 5, 7, 11, 13, 17, 19
    };

    for (unsigned round = 0; round < rounds; ++round) {
        cpp_int a;
        if (round < sizeof(fixedBases) / sizeof(fixedBases[0])) {
            a = fixedBases[round];
        } else {
            a = cpp_int(rng());
            a = 2 + (a % (n - 3));
        }

        if (a >= n) {
            continue;
        }

        cpp_int x = modPow(a, d, n);
        if (x == 1 || x == n - 1) {
            continue;
        }

        bool passed = false;
        for (unsigned i = 1; i < s; ++i) {
            x = (x * x) % n;
            if (x == n - 1) {
                passed = true;
                break;
            }
        }

        if (!passed) {
            return false;
        }
    }

    return true;
}

cpp_int generateBlumPrime(unsigned bits, std::mt19937_64& rng)
{
    while (true) {
        cpp_int candidate = randomBits(bits, rng);
        candidate |= 3; // candidate == 3 (mod 4)

        if (isProbablePrime(candidate, 20, rng)) {
            return candidate;
        }
    }
}

cpp_int bytesToInt(const Byte* data, std::size_t size)
{
    cpp_int value = 0;
    for (std::size_t i = 0; i < size; ++i) {
        value <<= 8;
        value += data[i];
    }
    return value;
}

void intToFixedBytes(cpp_int value, Byte* output, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        const std::size_t pos = size - 1 - i;
        output[pos] = static_cast<Byte>((value & 0xFF).convert_to<unsigned>());
        value >>= 8;
    }
}

std::uint16_t checksum(const std::vector<Byte>& data)
{
    std::uint32_t result = 0;
    for (Byte value : data) {
        result = (result * 257u + value) & 0xFFFFu;
    }
    return static_cast<std::uint16_t>(result);
}

cpp_int encodeChunk(const std::vector<Byte>& chunk)
{
    if (chunk.size() > MAX_PAYLOAD) {
        throw std::runtime_error("Payload block is too large");
    }

    std::array<Byte, RECORD_SIZE> record{};
    record[0] = 0xA5;
    record[1] = static_cast<Byte>(chunk.size());

    std::copy(chunk.begin(), chunk.end(), record.begin() + 2);

    const std::uint16_t sum = checksum(chunk);
    record[2 + chunk.size()] = static_cast<Byte>(sum >> 8);
    record[3 + chunk.size()] = static_cast<Byte>(sum);

    return bytesToInt(record.data(), record.size());
}

bool decodeChunk(const cpp_int& message,
                 std::vector<Byte>& chunk)
{
    std::array<Byte, RECORD_SIZE> record{};
    intToFixedBytes(message, record.data(), record.size());

    if (record[0] != 0xA5) {
        return false;
    }

    const std::size_t length = record[1];
    if (length > MAX_PAYLOAD) {
        return false;
    }

    const std::uint16_t stored =
        static_cast<std::uint16_t>(record[2 + length]) << 8 |
        record[3 + length];

    chunk.assign(record.begin() + 2,
                 record.begin() + 2 + length);

    if (checksum(chunk) != stored) {
        return false;
    }

    return true;
}

std::array<cpp_int, 4> calculateRoots(const cpp_int& c,
                                      const cpp_int& p,
                                      const cpp_int& q)
{
    const cpp_int mp = modPow(c, (p + 1) / 4, p);
    const cpp_int mq = modPow(c, (q + 1) / 4, q);

    cpp_int yp;
    cpp_int yq;
    extendedGcd(p, q, yp, yq);

    const cpp_int n = p * q;

    const cpp_int r = modPositive(
        yp * p * mq + yq * q * mp,
        n);

    const cpp_int s = modPositive(
        yp * p * mq - yq * q * mp,
        n);

    return {
        r,
        modPositive(n - r, n),
        s,
        modPositive(n - s, n)
    };
}

bool saveTextValue(const std::string& fileName, const cpp_int& value)
{
    std::ofstream file(fileName);
    if (!file) {
        return false;
    }
    file << value << '\n';
    return static_cast<bool>(file);
}

} // namespace

RabinKeyPair generateKeyPair(unsigned modulusBits)
{
    if (modulusBits < 32 || modulusBits % 2 != 0) {
        throw std::invalid_argument(
            "Modulus size must be an even number >= 32");
    }

    std::random_device rd;
    std::seed_seq seed{
        rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()
    };
    std::mt19937_64 rng(seed);

    const unsigned primeBits = modulusBits / 2;

    while (true) {
        const cpp_int p = generateBlumPrime(primeBits, rng);
        const cpp_int q = generateBlumPrime(primeBits, rng);

        if (p == q) {
            continue;
        }

        return {p, q, p * q};
    }
}

bool savePublicKey(const std::string& fileName, const cpp_int& n)
{
    return saveTextValue(fileName, n);
}

bool savePrivateKey(const std::string& fileName,
                    const cpp_int& p,
                    const cpp_int& q)
{
    std::ofstream file(fileName);
    if (!file) {
        return false;
    }

    file << p << '\n' << q << '\n';
    return static_cast<bool>(file);
}

bool loadPublicKey(const std::string& fileName, cpp_int& n)
{
    std::ifstream file(fileName);
    if (!file) {
        return false;
    }

    file >> n;
    return static_cast<bool>(file) && n > 0;
}

bool loadPrivateKey(const std::string& fileName,
                    cpp_int& p,
                    cpp_int& q)
{
    std::ifstream file(fileName);
    if (!file) {
        return false;
    }

    file >> p >> q;
    if (!file || p <= 0 || q <= 0 || p == q) {
        return false;
    }

    return p % 4 == 3 && q % 4 == 3;
}

bool encryptFile(const std::string& inputFile,
                 const std::string& outputFile,
                 const std::string& publicKeyFile)
{
    std::vector<Byte> plain;
    if (!readBinaryFile(inputFile, plain)) {
        return false;
    }

    cpp_int n;
    if (!loadPublicKey(publicKeyFile, n)) {
        return false;
    }

    // RECORD_SIZE = 31 bytes, therefore blocks are below 248 bits.
    // A 256-bit Rabin modulus is sufficient for the default lab setup.
    const cpp_int minimumModulus = cpp_int(1) << (RECORD_SIZE * 8 - 1);
    if (n <= minimumModulus) {
        return false;
    }

    std::ofstream output(outputFile, std::ios::binary);
    if (!output) {
        return false;
    }

    output << FILE_MAGIC << '\n';
    output << n << '\n';
    output << plain.size() << '\n';

    const std::size_t blockCount =
        plain.empty() ? 0 : (plain.size() + MAX_PAYLOAD - 1) / MAX_PAYLOAD;
    output << blockCount << '\n';

    for (std::size_t pos = 0; pos < plain.size(); pos += MAX_PAYLOAD) {
        const std::size_t count =
            std::min(MAX_PAYLOAD, plain.size() - pos);

        std::vector<Byte> chunk(
            plain.begin() + static_cast<std::ptrdiff_t>(pos),
            plain.begin() + static_cast<std::ptrdiff_t>(pos + count));

        const cpp_int m = encodeChunk(chunk);
        if (m >= n) {
            return false;
        }

        const cpp_int c = (m * m) % n;
        output << c << '\n';
    }

    return static_cast<bool>(output);
}

bool decryptFile(const std::string& inputFile,
                 const std::string& outputFile,
                 const std::string& privateKeyFile)
{
    cpp_int p;
    cpp_int q;
    if (!loadPrivateKey(privateKeyFile, p, q)) {
        return false;
    }

    std::ifstream input(inputFile, std::ios::binary);
    if (!input) {
        return false;
    }

    std::string magic;
    std::getline(input, magic);
    if (magic != FILE_MAGIC) {
        return false;
    }

    cpp_int n;
    std::size_t originalSize = 0;
    std::size_t blockCount = 0;

    input >> n >> originalSize >> blockCount;
    if (!input || n != p * q) {
        return false;
    }

    std::string ignored;
    std::getline(input, ignored);

    std::vector<Byte> plain;
    plain.reserve(originalSize);

    for (std::size_t block = 0; block < blockCount; ++block) {
        std::string line;
        if (!std::getline(input, line)) {
            return false;
        }

        std::istringstream parser(line);
        cpp_int c;
        parser >> c;
        if (!parser || c < 0 || c >= n) {
            return false;
        }

        const std::array<cpp_int, 4> roots =
            calculateRoots(c, p, q);

        std::vector<Byte> chunk;
        bool found = false;

        for (const cpp_int& root : roots) {
            if (decodeChunk(root, chunk)) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }

        plain.insert(plain.end(), chunk.begin(), chunk.end());
    }

    if (plain.size() != originalSize) {
        return false;
    }

    return writeBinaryFile(outputFile, plain);
}
