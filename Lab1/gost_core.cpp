#include "gost_core.h"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <vector>

const uint8_t S_BOX[8][16] = {
    {4, 10, 9, 2, 13, 8, 0, 14, 6, 11, 1, 12, 7, 15, 5, 3},
    {14, 11, 4, 12, 6, 13, 15, 10, 2, 3, 8, 1, 0, 7, 5, 9},
    {5, 8, 1, 13, 10, 3, 4, 2, 14, 15, 12, 7, 6, 0, 9, 11},
    {7, 13, 10, 1, 0, 8, 9, 15, 14, 4, 6, 12, 11, 2, 5, 3},
    {6, 12, 7, 1, 5, 15, 13, 8, 4, 10, 9, 14, 0, 3, 11, 2},
    {4, 11, 10, 0, 7, 2, 1, 13, 3, 6, 8, 5, 9, 12, 15, 14},
    {13, 11, 4, 1, 3, 15, 5, 9, 0, 10, 14, 7, 6, 8, 2, 12},
    {1, 15, 13, 0, 5, 7, 10, 4, 9, 2, 3, 14, 6, 11, 8, 12}
};

GOST28147::GOST28147(const uint8_t key[32]) {
    for (int i = 0; i < 8; ++i) {
        subkeys[i] = (key[4*i] | (key[4*i+1]<<8) | (key[4*i+2]<<16) | (key[4*i+3]<<24));
    }
}

void GOST28147::round(uint32_t &N1, uint32_t &N2, uint32_t key) const {
    uint32_t S = N1 + key;
    uint32_t transformed = 0;

    for (int i = 0; i < 8; ++i) {
        transformed |= ((uint32_t)S_BOX[i][(S >> (4 * i)) & 0xF] << (4 * i));
    }

    transformed = (transformed << 11) | (transformed >> 21);

    uint32_t nextN2 = N2 ^ transformed;

    N2 = N1;
    N1 = nextN2;
}

void GOST28147::encryptBlock(uint32_t &left, uint32_t &right) const {
    uint32_t n1 = left, n2 = right;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 8; ++j) {
            round(n1, n2, subkeys[j]);
        }
    }
    for (int j = 0; j < 8; ++j) {
        round(n1, n2, subkeys[7 - j]);
    }

    left = n2; right = n1;
}

void GOST28147::encryptBlock16(uint32_t &left, uint32_t &right) const {
    uint32_t n1 = left, n2 = right;

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 8; ++j) {
            round(n1, n2, subkeys[j]);
        }
    }

    left = n2; right = n1;
}

void GOST28147::decryptBlock(uint32_t &left, uint32_t &right) const {
    uint32_t n1 = left, n2 = right;

    for (int j = 0; j < 8; ++j) {
        round(n1, n2, subkeys[j]);
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 8; ++j) {
            round(n1, n2, subkeys[7 - j]);
        }
    }

    left = n2; right = n1;
}

uint32_t GOST28147::calculateMAC(const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> b = data;
    while (b.size() % 8 != 0) {
        b.push_back(0);
    }
    uint32_t l = 0, r = 0;
    for (size_t i = 0; i < b.size(); i += 8) {
        uint32_t b1, b2; std::memcpy(&b1, &b[i], 4); std::memcpy(&b2, &b[i+4], 4);
        l ^= b1;
        r ^= b2;
        encryptBlock16(l, r);
    }
    return l;
}

void processGost(const std::string& mode, bool encrypt, const std::string& input, const std::string& output, const uint8_t key[32], uint64_t iv) {
    GOST28147 cipher(key);
    std::ifstream inFile(input, std::ios::binary);
    std::ofstream outFile(output, std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());

    if (mode == "ECB") { // simple change mode
        if (encrypt) {
            size_t padLen = 8 - (buffer.size() % 8);
            for(size_t i = 0; i < padLen; ++i) buffer.push_back(static_cast<uint8_t>(padLen));
        }
        for (size_t i = 0; i < buffer.size(); i += 8) {
            uint32_t left, right;
            std::memcpy(&left, &buffer[i], 4);
            std::memcpy(&right, &buffer[i + 4], 4);
            if (encrypt) cipher.encryptBlock(left, right);
            else cipher.decryptBlock(left, right);
            std::memcpy(&buffer[i], &left, 4);
            std::memcpy(&buffer[i + 4], &right, 4);
        }
        if (!encrypt) {
            uint8_t pad = buffer.back();
            if (pad > 0 && pad <= 8) {
                bool valid = true;
                for(size_t i = 0; i < pad; ++i) if(buffer[buffer.size() - 1 - i] != pad) valid = false;
                if (valid) buffer.resize(buffer.size() - pad);
            }
        }
    } else if (mode == "OFB") { // gamma syclic mode
        uint64_t reg = iv;
        for (size_t i = 0; i < buffer.size(); i += 8) {
            uint32_t left = reg & 0xFFFFFFFF;
            uint32_t right = reg >> 32;
            cipher.encryptBlock(left, right);

            uint64_t gamma = (static_cast<uint64_t>(left)) | (static_cast<uint64_t>(right) << 32);

            for (size_t j = 0; j < 8 && (i + j) < buffer.size(); ++j) {
                buffer[i + j] ^= (gamma >> (8 * j)) & 0xFF;
            }

            reg = gamma;
        }
    } else if (mode == "CFB") { // next gamma is previous encrypted text
        uint64_t reg = iv;
        for (size_t i = 0; i < buffer.size(); i += 8) {
            uint32_t left = reg & 0xFFFFFFFF;
            uint32_t right = reg >> 32;
            cipher.encryptBlock(left, right);
            uint64_t gamma = (static_cast<uint64_t>(left)) | (static_cast<uint64_t>(right) << 32);
            uint64_t block = 0;
            std::memcpy(&block, &buffer[i], std::min(size_t(8), buffer.size() - i));
            uint64_t encrypted = block ^ gamma;
            if (encrypt) {
                std::memcpy(&buffer[i], &encrypted, std::min(size_t(8), buffer.size() - i));
                reg = encrypted;
            } else {
                reg = block;
                std::memcpy(&buffer[i], &encrypted, std::min(size_t(8), buffer.size() - i));
            }
        }
    }
    outFile.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
}
