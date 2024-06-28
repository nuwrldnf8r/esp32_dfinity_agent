#include "uECC.h"
#include "esp_system.h"
#include "keypair.h"
#include "esp_system.h"
#include "utils.h"
#include <stdexcept>
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <cstring>
#include <EEPROM.h>
#include "mbedtls/sha256.h"
#include <mbedtls/pk.h>
#include <mbedtls/pem.h>
#include <mbedtls/error.h>
#include <mbedtls/ecp.h>
#include <iostream>


#define EEPROM_SIZE 100
#define MARKER_ADDRESS 0
#define MARKER 0xABFF

const char base32Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

std::string base32_encode(const unsigned char* data, size_t length) {
    std::string result;
    int buffer = data[0];
    int next = 1;
    int bitsLeft = 8;

    while (bitsLeft > 0 || next < length) {
        if (bitsLeft < 5) {
            if (next < length) {
                buffer <<= 8;
                buffer |= data[next++] & 0xFF;
                bitsLeft += 8;
            } else {
                int pad = 5 - bitsLeft;
                buffer <<= pad;
                bitsLeft += pad;
            }
        }

        int index = (buffer >> (bitsLeft - 5)) & 0x1F;
        bitsLeft -= 5;
        result += base32Alphabet[index];
    }

    return result;
}

int esp_random_function(uint8_t *dest, unsigned size) {
    esp_fill_random(dest, size);
    return 1; 
}

void writeMarker() {
    EEPROM.write(MARKER_ADDRESS, (MARKER >> 8) & 0xFF);
    EEPROM.write(MARKER_ADDRESS + 1, MARKER & 0xFF);
    EEPROM.commit();
}

bool checkDataExists() {
    uint16_t marker = (EEPROM.read(MARKER_ADDRESS) << 8) | EEPROM.read(MARKER_ADDRESS + 1);
    return marker == MARKER;
}

void writeVectorToEEPROM(const std::vector<uint8_t>& data, int startAddress) {
    for (size_t i = 0; i < data.size(); ++i) {
        EEPROM.write(startAddress + i, data[i]);
    }
    EEPROM.commit();
}

std::vector<uint8_t> readVectorFromEEPROM(size_t dataSize, int startAddress) {
    std::vector<uint8_t> data(dataSize);
    for (size_t i = 0; i < dataSize; ++i) {
        data[i] = EEPROM.read(startAddress + i);
    }
    return data;
}


std::vector<unsigned char> readPrivateKeyFromStorage(){
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return std::vector<unsigned char>();  // Return an empty vector
    }
    std::vector<uint8_t> private_key_buf = readVectorFromEEPROM(32, 2);
    return std::vector<unsigned char>(private_key_buf.begin(), private_key_buf.end());
}


Keypair::Keypair(){}

void Keypair::initialize(){
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

    // Convert the public key to DER format
    // Add a 0x04 byte to the start of the public key
    std::vector<uint8_t> public_key_with_prefix;
    public_key_with_prefix.push_back(0x04);
    public_key_with_prefix.insert(public_key_with_prefix.end(), public_key, public_key + public_key_size);

    // Convert the public key to DER format
    std::vector<uint8_t> der_encoded_public_key = der_encode_public_key(public_key_with_prefix);
    _public_key_der.assign(der_encoded_public_key.begin(), der_encoded_public_key.end());
   
}

void Keypair::initialize(bool from_storage){
    Serial.println("Starting initialization with storage flag...");
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Failed to initialize EEPROM");
        return;
    }
    if (from_storage) {
        bool exists = checkDataExists();
        printf("Data exists: %d\n", exists);
        if(!exists){
            printf("Writing marker\n");
            writeMarker();
            initialize();
            writeVectorToEEPROM(_private_key, 2);
            
        } else {
            std::vector<unsigned char> private_key_buf = readPrivateKeyFromStorage();
            initialize(private_key_buf);
        }
      
    } else {
        initialize();
    }
    EEPROM.end();
}

#include <vector>
#include <stdexcept>

void Keypair::initialize(const std::vector<unsigned char>& private_key_buf){
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

    // Convert the public key to DER format
    // Add a 0x04 byte to the start of the public key
    std::vector<uint8_t> public_key_with_prefix;
    public_key_with_prefix.push_back(0x04);
    public_key_with_prefix.insert(public_key_with_prefix.end(), public_key, public_key + public_key_size);

    // Convert the public key to DER format
    std::vector<uint8_t> der_encoded_public_key = der_encode_public_key(public_key_with_prefix);
    _public_key_der.assign(der_encoded_public_key.begin(), der_encoded_public_key.end());
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

std::vector<uint8_t> Keypair::getPrincipal() const {
   // Compute SHA-224 hash
    unsigned char hash[28];
    mbedtls_sha256(_public_key_der.data(), _public_key_der.size(), hash, 1);

    std::vector<uint8_t> hash_vector(hash, hash + sizeof(hash));
    hash_vector.push_back(0x02);
    return hash_vector;
}


// Helper function to encode length in DER format
std::vector<uint8_t> encode_length(size_t length) {
    std::vector<uint8_t> encoded_length;
    if (length < 128) {
        encoded_length.push_back(static_cast<uint8_t>(length));
    } else {
        std::vector<uint8_t> len_bytes;
        while (length > 0) {
            len_bytes.push_back(static_cast<uint8_t>(length & 0xFF));
            length >>= 8;
        }
        encoded_length.push_back(static_cast<uint8_t>(0x80 | len_bytes.size()));
        for (auto it = len_bytes.rbegin(); it != len_bytes.rend(); ++it) {
            encoded_length.push_back(*it);
        }
    }
    return encoded_length;
}

// Helper function to encode OID
std::vector<uint8_t> encode_oid(const std::vector<uint8_t>& oid) {
    std::vector<uint8_t> encoded_oid = {0x06}; // OID tag
    auto encoded_length = encode_length(oid.size());
    encoded_oid.insert(encoded_oid.end(), encoded_length.begin(), encoded_length.end());
    encoded_oid.insert(encoded_oid.end(), oid.begin(), oid.end());
    return encoded_oid;
}

// Function to DER encode EC public key for secp256k1
std::vector<uint8_t> Keypair::der_encode_public_key(const std::vector<uint8_t>& public_key) {
    // OID for id-ecPublicKey and secp256k1
    std::vector<uint8_t> oid_ecPublicKey = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01};
    std::vector<uint8_t> oid_secp256k1 = {0x2B, 0x81, 0x04, 0x00, 0x0A};

    // AlgorithmIdentifier sequence
    std::vector<uint8_t> algorithm_identifier = {0x30}; // SEQUENCE tag
    std::vector<uint8_t> encoded_oid_ecPublicKey = encode_oid(oid_ecPublicKey);
    std::vector<uint8_t> encoded_oid_secp256k1 = encode_oid(oid_secp256k1);
    std::vector<uint8_t> algorithm_identifier_content;
    algorithm_identifier_content.insert(algorithm_identifier_content.end(), encoded_oid_ecPublicKey.begin(), encoded_oid_ecPublicKey.end());
    algorithm_identifier_content.insert(algorithm_identifier_content.end(), encoded_oid_secp256k1.begin(), encoded_oid_secp256k1.end());
    auto algorithm_identifier_length = encode_length(algorithm_identifier_content.size());
    algorithm_identifier.insert(algorithm_identifier.end(), algorithm_identifier_length.begin(), algorithm_identifier_length.end());
    algorithm_identifier.insert(algorithm_identifier.end(), algorithm_identifier_content.begin(), algorithm_identifier_content.end());

    // SubjectPublicKey BIT STRING
    std::vector<uint8_t> subject_public_key = {0x03}; // BIT STRING tag
    std::vector<uint8_t> public_key_with_prefix = {0x00}; // Number of unused bits
    public_key_with_prefix.insert(public_key_with_prefix.end(), public_key.begin(), public_key.end());
    auto public_key_length = encode_length(public_key_with_prefix.size());
    subject_public_key.insert(subject_public_key.end(), public_key_length.begin(), public_key_length.end());
    subject_public_key.insert(subject_public_key.end(), public_key_with_prefix.begin(), public_key_with_prefix.end());

    // SubjectPublicKeyInfo sequence
    std::vector<uint8_t> subject_public_key_info = {0x30}; // SEQUENCE tag
    std::vector<uint8_t> subject_public_key_info_content;
    subject_public_key_info_content.insert(subject_public_key_info_content.end(), algorithm_identifier.begin(), algorithm_identifier.end());
    subject_public_key_info_content.insert(subject_public_key_info_content.end(), subject_public_key.begin(), subject_public_key.end());
    auto subject_public_key_info_length = encode_length(subject_public_key_info_content.size());
    subject_public_key_info.insert(subject_public_key_info.end(), subject_public_key_info_length.begin(), subject_public_key_info_length.end());
    subject_public_key_info.insert(subject_public_key_info.end(), subject_public_key_info_content.begin(), subject_public_key_info_content.end());

    return subject_public_key_info;
}