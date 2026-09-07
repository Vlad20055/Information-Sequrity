#pragma once
#include "utils.h"
#include <array>
#include <vector>

constexpr size_t BLOCK_SIZE = 16; // 128 бит
constexpr size_t KEY_SIZE   = 32; // 256 бит

using Block = std::array<Byte, BLOCK_SIZE>;
using Key   = std::array<Byte, KEY_SIZE>;

Block beltEncrypt(const Block& input, const Key& key);
Block beltDecrypt(const Block& input, const Key& key);

// padding (PKCS#7)
void addPadding(std::vector<Byte>& data);
bool removePadding(std::vector<Byte>& data);
