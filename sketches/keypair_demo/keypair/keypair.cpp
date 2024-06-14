#include "uECC.h"
#include <stdexcept>
#include <stdint.h>
#include <stddef.h>
#include "esp_system.h"
#include <vector>
#include <cstring>

int esp_random_function(uint8_t *dest, unsigned size) {
    esp_fill_random(dest, size);
    return 1; 
}


Keypair::Keypair() {
    _is_initialized = true;
    uECC_set_rng(esp_random_function);
    uECC_Curve curve = uECC_secp256k1();
    size_t private_key_size = uECC_curve_private_key_size(curve);
    size_t public_key_size = uECC_curve_public_key_size(curve);
    uint8_t private_key[private_key_size];
    uint8_t public_key[public_key_size];

    // Fill the private key buffer with random data
    esp_fill_random(private_key, private_key_size);

    // Compute the public key from the private key
    if (!uECC_compute_public_key(private_key, public_key, curve)) {
        memset(private_key, 0, private_key_size);
        throw std::runtime_error("Failed to generate key pair");
    }

    // Assign private key to buffer and zero out sensitive data
    _private_key.assign(private_key, private_key + private_key_size);
    memset(private_key, 0, private_key_size);  // Zero out the private key buffer

    // Assign public key to buffer
    _public_key.assign(public_key, public_key + public_key_size);
}

Keypair::Keypair(const std::vector<unsigned char>& private_key_buf) {
    _is_initialized = true;
    uECC_set_rng(esp_random_function);
    uECC_Curve curve = uECC_secp256k1();
    size_t private_key_size = uECC_curve_private_key_size(curve);
    size_t public_key_size = uECC_curve_public_key_size(curve);
    uint8_t private_key[private_key_size];
    uint8_t public_key[public_key_size];

    // Ensure the provided private key buffer is the correct size
    if (private_key_buf.size() != private_key_size) {
        throw std::runtime_error("Invalid private key size");
    }

    std::memcpy(private_key, private_key_buf.data(), private_key_size);

    // Compute the public key from the private key
    if (!uECC_compute_public_key(private_key, public_key, curve)) {
        memset(private_key, 0, private_key_size);
        throw std::runtime_error("Failed to compute public key");
    }

    // Assign private key to buffer and zero out sensitive data
    _private_key.assign(private_key, private_key + private_key_size);
    memset(private_key, 0, private_key_size);  // Zero out the private key buffer

    // Assign public key to buffer
    _public_key.assign(public_key, public_key + public_key_size);
}

std::vector<unsigned char> Keypair::sign(const std::vector<unsigned char>& message) {
    if (!_is_initialized) {
        throw std::runtime_error("Keypair not initialized");
    }
    //uECC_set_rng(esp_random_function);
    uECC_Curve curve = uECC_secp256k1();
    size_t signature_size = uECC_curve_private_key_size(curve) * 2;
    uint8_t signature[signature_size];

    // Ensure message size does not exceed maximum allowed size
    if (message.size() > UINT16_MAX) {
        throw std::runtime_error("Message size too large to sign");
    }

    // Sign the message
    if (!uECC_sign(_private_key.data(), message.data(), message.size(), signature, curve)) {
        throw std::runtime_error("Failed to sign message. Make sure the private key and message are correct.");
    }

    return std::vector<unsigned char>(signature, signature + signature_size);
}

bool Keypair::verify(const std::vector<unsigned char>& message, const std::vector<uint8_t>& public_key, const std::vector<unsigned char>& signature) {
    uECC_set_rng(esp_random_function);
    uECC_Curve curve = uECC_secp256k1();

    // Ensure message size does not exceed maximum allowed size
    if (message.size() > UINT16_MAX) {
        throw std::runtime_error("Message size too large to verify");
    }

    // Verify the signature
    return uECC_verify(public_key.data(), message.data(), message.size(), signature.data(), curve);
}