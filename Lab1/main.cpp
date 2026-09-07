// Usage: ./gost <mode: ECB|OFB|CFB> <enc|dec> <input> <output>

// ./build/gost ECB enc tests/test1.txt tests/test1.bin
// ./build/gost ECB dec tests/test1.bin tests/decr1.txt

#include "gost_core.h"
#include <iostream>
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cout << "Usage: ./gost <mode: ECB|OFB|CFB> <enc|dec> <input> <output>\n";
        return 1;
    }
    
    uint8_t key[32] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 
                       0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                       0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
                       0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    uint64_t iv = 0x1234567890ABCDEF;
    
    std::string mode = argv[1];
    bool encrypt = (std::string(argv[2]) == "enc");
    std::string input = argv[3];
    std::string output = argv[4];

    try {
        processGost(mode, encrypt, input, output, key, iv);
        
        std::ifstream outFile(output, std::ios::binary);
        std::vector<uint8_t> result((std::istreambuf_iterator<char>(outFile)), std::istreambuf_iterator<char>());
        GOST28147 cipher(key);
        std::cout << "MAC: " << std::hex << cipher.calculateMAC(result) << std::endl;
        
        std::cout << "Successfully processed " << input << " -> " << output << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
