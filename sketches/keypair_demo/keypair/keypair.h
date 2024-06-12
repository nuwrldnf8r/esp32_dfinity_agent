#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <vector>
#include <string>
#include <utility>
#include "mbedtls/ctr_drbg.h"

class Keypair {
public:
    mbedtls_ctr_drbg_context ctr_drbg;
    Keypair();
    Keypair(const std::string& private_random_number);
    std::string getRandom();
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateKeyPair(const std::string& private_random_number); 
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateKeyPair(const std::vector<uint8_t>& private_key_vec);
    void generate(const std::string& private_random_number);
    void generate(const std::vector<uint8_t>& private_key_vec);
    std::vector<uint8_t> getPublicKey();
    std::vector<uint8_t> getPrivateKey();
    std::string getPublicKeyString();
    std::string getPrivateKeyString();
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message);
    bool verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature);
    std::vector<uint8_t> sign(const std::string& message);
    bool verify(const std::string& message, const std::vector<uint8_t>& signature);

private:
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> _keypair;
};

#endif // KEYPAIR_H

