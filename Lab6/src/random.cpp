#include "random.h"

#include <cerrno>
#include <stdexcept>
#include <sys/random.h>
#include <vector>

namespace {

void fillRandom(std::vector<uint8_t>& buf) {
    size_t done = 0;
    while (done < buf.size()) {
        const ssize_t got =
            getrandom(buf.data() + done, buf.size() - done, 0);
        if (got < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("ошибка getrandom()");
        }
        done += size_t(got);
    }
}

}

BigInt randomBelow(const BigInt& q) {
    const size_t bits = q.bitLength();
    std::vector<uint8_t> buf((bits + 7) / 8);
    while (true) {
        fillRandom(buf);
        if (bits % 8) buf.back() &= uint8_t((1u << (bits % 8)) - 1);
        const BigInt k = BigInt::fromBytesLE(buf);
        if (!k.isZero() && k < q) return k;
    }
}
