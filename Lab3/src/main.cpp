// ./build/rabin keygen 256 tests/public1.key tests/private1.key
// ./build/rabin enc tests/test1.txt tests/test1.rbn tests/public1.key
// ./build/rabin dec tests/test1.rbn tests/dec1.txt tests/private1.key

#include "rabin.hpp"
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

void printUsage(const char* programName)
{
    std::cerr
        << "Usage:\n"
        << "  " << programName << " keygen <bits> <public-key> <private-key>\n"
        << "  " << programName << " enc <input> <output> <public-key>\n"
        << "  " << programName << " dec <input> <output> <private-key>\n\n"
        << "Parameters:\n"
        << "  keygen : Generate Rabin keys. <bits> is the modulus size.\n"
        << "  enc    : Encrypt the input file with the public key.\n"
        << "  dec    : Decrypt the input file with the private key.\n\n"
        << "Examples:\n"
        << "  " << programName << " keygen 256 public.key private.key\n"
        << "  " << programName << " enc secret.txt secret.rbn public.key\n"
        << "  " << programName << " dec secret.rbn restored.txt private.key\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string command = argv[1];

    try {
        if (command == "keygen") {
            if (argc != 5) {
                printUsage(argv[0]);
                return 1;
            }

            const unsigned bits = static_cast<unsigned>(std::stoul(argv[2]));
            const std::string publicKeyFile = argv[3];
            const std::string privateKeyFile = argv[4];

            const RabinKeyPair keys = generateKeyPair(bits);

            if (!savePublicKey(publicKeyFile, keys.n)) {
                std::cerr << "Error: cannot write public key file.\n";
                return 1;
            }

            if (!savePrivateKey(privateKeyFile, keys.p, keys.q)) {
                std::cerr << "Error: cannot write private key file.\n";
                return 1;
            }

            std::cout << "Keys generated successfully.\n";
            return 0;
        }

        if (command == "enc") {
            if (argc != 5) {
                printUsage(argv[0]);
                return 1;
            }

            const bool success = encryptFile(
                argv[2],
                argv[3],
                argv[4]);

            if (!success) {
                std::cerr << "Error: encryption failed.\n";
                return 1;
            }

            std::cout << "Encryption completed successfully.\n";
            return 0;
        }

        if (command == "dec") {
            if (argc != 5) {
                printUsage(argv[0]);
                return 1;
            }

            const bool success = decryptFile(
                argv[2],
                argv[3],
                argv[4]);

            if (!success) {
                std::cerr << "Error: decryption failed.\n";
                return 1;
            }

            std::cout << "Decryption completed successfully.\n";
            return 0;
        }

        std::cerr << "Error: unknown command '" << command << "'.\n";
        printUsage(argv[0]);
        return 1;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }
}
