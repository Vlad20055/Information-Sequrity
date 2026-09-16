#pragma once

#include <cstdint>
#include <string>
#include <vector>

using Byte = std::uint8_t;

bool readBinaryFile(const std::string& fileName, std::vector<Byte>& data);
bool writeBinaryFile(const std::string& fileName, const std::vector<Byte>& data);
