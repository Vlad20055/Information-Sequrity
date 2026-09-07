// Usage: ./belt <mode: ECB|CFB> <enc|dec> <input> <output>

// ./build/belt ECB enc tests/test1.txt tests/test1.bin
// ./build/belt ECB dec tests/test1.bin tests/decr1.txt

// ./build/belt CFB enc tests/test1.txt tests/test1.bin
// ./build/belt CFB dec tests/test1.bin tests/decr1.txt

#include <iostream>
#include <algorithm>
#include <string>
#include "belt.h"
#include "utils.h"

const std::string FIXED_KEY_HEX = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
const std::string FIXED_IV_HEX = "00112233445566778899AABBCCDDEEFF";

static Key getFixedKey() {
    Key key{};
    hexToBytes(FIXED_KEY_HEX, key.data(), key.size());
    return key;
}

static Block getFixedIV() {
    Block iv{};
    hexToBytes(FIXED_IV_HEX, iv.data(), iv.size());
    return iv;
}

static bool simpleReplacement(bool decrypt, const std::string& inputFile, const std::string& outputFile) {
    Key key = getFixedKey();
    std::vector<Byte> data;

    if (!readFile(inputFile, data)) {
        std::cerr << "Error: could not open input file '" << inputFile << "'\n";
        return false;
    }

    if (!decrypt) { // encrypt
        addPadding(data);
        std::vector<Byte> encrypted(data.size());
        
        for (size_t pos = 0; pos < data.size(); pos += BLOCK_SIZE) {
            Block inputBlock{};
            std::copy_n(data.begin() + pos, BLOCK_SIZE, inputBlock.begin());
            Block outputBlock = beltEncrypt(inputBlock, key);
            std::copy(outputBlock.begin(), outputBlock.end(), encrypted.begin() + pos);
        }
        
        if (!writeFile(outputFile, encrypted)) {
            std::cerr << "Error: could not write to file '" << outputFile << "'\n";
            return false;
        }
        std::cout << "[OK] Encryption (ECB) completed successfully.\n";
    } else { // decrypt
        if (data.empty() || data.size() % BLOCK_SIZE != 0) {
            std::cerr << "Error: ciphertext size must be a multiple of 16 bytes.\n";
            return false;
        }
        
        std::vector<Byte> decrypted(data.size());
        
        for (size_t pos = 0; pos < data.size(); pos += BLOCK_SIZE) {
            Block inputBlock{};
            std::copy_n(data.begin() + pos, BLOCK_SIZE, inputBlock.begin());
            Block outputBlock = beltDecrypt(inputBlock, key);
            std::copy(outputBlock.begin(), outputBlock.end(), decrypted.begin() + pos);
        }
        
        if (!removePadding(decrypted)) {
            std::cerr << "Decryption error: invalid key or corrupted data (padding error).\n";
            return false;
        }
        
        if (!writeFile(outputFile, decrypted)) {
            std::cerr << "Error: could not write to file '" << outputFile << "'\n";
            return false;
        }
        std::cout << "[OK] Decryption (ECB) completed successfully.\n";
    }
    return true;
}

static bool cipherFeedback(bool decrypt, const std::string& inputFile, const std::string& outputFile) {
    Key key = getFixedKey(); 
    Block iv = getFixedIV();
    std::vector<Byte> data;

    if (!readFile(inputFile, data)) {
        std::cerr << "Error: could not open input file '" << inputFile << "'\n";
        return false;
    }

    std::vector<Byte> result(data.size());
    Block previous = iv;

    for (size_t pos = 0; pos < data.size(); pos += BLOCK_SIZE) {
        Block gamma = beltEncrypt(previous, key);
        size_t currentSize = std::min(BLOCK_SIZE, data.size() - pos);
        Block currentBlock{};

        for (size_t j = 0; j < currentSize; ++j) {
            currentBlock[j] = data[pos + j] ^ gamma[j];
            result[pos + j] = currentBlock[j];
        }

        if (!decrypt) {
            for (size_t j = 0; j < currentSize; ++j) {
                previous[j] = result[pos + j];
            }
        } else {
            for (size_t j = 0; j < currentSize; ++j) {
                previous[j] = data[pos + j];
            }
        }
    }

    if (!writeFile(outputFile, result)) {
        std::cerr << "Error: could not write to file '" << outputFile << "'\n";
        return false;
    }
    
    std::cout << "[OK] " << (decrypt ? "Decryption" : "Encryption")
              << " (CFB) completed successfully.\n";
    return true;
}


void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " <mode: ECB|CFB> <enc|dec> <input> <output>\n"
              << "Parameters:\n"
              << "  mode   : ECB (Electronic Codebook) or CFB (Cipher Feedback)\n"
              << "  action : enc (Encrypt) or dec (Decrypt)\n"
              << "  input  : Path to the input file\n"
              << "  output : Path to the output file\n\n"
              << "Example: " << programName << " ECB enc secret.txt secret.enc\n";
}


int main(int argc, char* argv[]) {
    if (argc != 5) {
        printUsage(argv[0]);
        return 1;
    }

    // args
    std::string modeStr = argv[1];
    std::string opStr   = argv[2];
    std::string inFile  = argv[3];
    std::string outFile = argv[4];

    if (modeStr != "ECB" && modeStr != "CFB") {
        std::cerr << "Error: unknown mode '" << modeStr << "'. Valid modes: ECB, CFB.\n";
        return 1;
    }

    if (opStr != "enc" && opStr != "dec") {
        std::cerr << "Error: unknown action '" << opStr << "'. Valid actions: enc, dec.\n";
        return 1;
    }

    bool decrypt = (opStr == "dec");
    bool success = false;

    // run
    if (modeStr == "ECB") {
        success = simpleReplacement(decrypt, inFile, outFile);
    } else if (modeStr == "CFB") {
        success = cipherFeedback(decrypt, inFile, outFile);
    }

    return success ? 0 : 1;
}
