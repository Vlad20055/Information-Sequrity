#ifndef GOST_CORE_H
#define GOST_CORE_H

#include <vector>
#include <cstdint>
#include <string>

class GOST28147 {
public:
    GOST28147(const uint8_t key[32]);
    void encryptBlock(uint32_t &left, uint32_t &right) const;
    void encryptBlock16(uint32_t &left, uint32_t &right) const;
    void decryptBlock(uint32_t &left, uint32_t &right) const;
    uint32_t calculateMAC(const std::vector<uint8_t>& data) const;

private:
    uint32_t subkeys[8];
    void round(uint32_t &N1, uint32_t &N2, uint32_t key) const;
};

void processGost(const std::string& mode, bool encrypt, const std::string& input, const std::string& output, 
                 const uint8_t key[32], uint64_t iv);

#endif
