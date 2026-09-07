#pragma once
#include <string>
#include <vector>
#include <cstdint>


using Byte = uint8_t;

bool hexToBytes(const std::string& hex, Byte* output, size_t outputSize);
bool readFile(const std::string& fileName, std::vector<Byte>& data);
bool writeFile(const std::string& fileName, const std::vector<Byte>& data);
