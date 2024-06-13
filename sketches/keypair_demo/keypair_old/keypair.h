#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <stdexcept>
#include <mbedtls/pk.h>

class Keypair {
public:
    Keypair();
    Keypair(const std::vector<unsigned char>& private_key_buf);
    std::vector<unsigned char> private_key() const { return _private_key_buf; }
    std::vector<unsigned char> public_key() const { return _public_key_buf; }
    std::vector<unsigned char> sign(const std::vector<unsigned char>& message);

private:
    std::vector<unsigned char> _private_key_buf;
    std::vector<unsigned char> _public_key_buf;
};

#endif // KEYPAIR_H

