#include "uECC.h"
#include <stdexcept>
#include <stdint.h>
#include <stddef.h>
#include "esp_system.h"

Keypair::Keypair(){
   

    // Compute the public key from the private key.
    uECC_Curve curve = uECC_secp256k1();
    size_t private_key_size = uECC_curve_private_key_size(curve);
    size_t public_key_size = uECC_curve_public_key_size(curve);
    uint8_t private_key[private_key_size];
    uint8_t public_key[public_key_size];

    //uint32_t rand_num = esp_random();
    esp_fill_random(private_key, private_key_size);

    //memcpy(private_key, &rand_num, sizeof(rand_num));
    if (!uECC_compute_public_key(private_key, public_key, curve)) {
        memset(private_key, 0, private_key_size);
        throw std::runtime_error("Failed to generate key pair");
    }
    
    _private_key_buf.assign(private_key, private_key + private_key_size);
    memset(private_key, 0, private_key_size);
    _public_key_buf.assign(public_key, public_key + public_key_size);
}


Keypair::Keypair(const std::vector<unsigned char>& private_key_buf) {
    uECC_Curve curve = uECC_secp256k1();
    uint8_t private_key[uECC_curve_private_key_size(curve)];
    uint8_t public_key[uECC_curve_public_key_size(curve)];

    size_t data_len = std::min(private_key_buf.size(), static_cast<std::size_t>(uECC_curve_private_key_size(curve)));
    for (size_t i = 0; i < data_len; i++) {
        private_key[i] = private_key_buf[i];
    }

    if (!uECC_compute_public_key(private_key, public_key, curve)) {
        throw std::runtime_error("Failed to compute public key");
    }

    _private_key_buf.assign(private_key, private_key + data_len);
    _public_key_buf.assign(public_key, public_key + data_len);
}