#include "utils.h"
#include "keypair.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/ecp.h>
#include <mbedtls/bignum.h>
#include <cstdint>
#include <map>
#include <cppcodec/base32_rfc4648.hpp>
#include <sstream>


using base32 = cppcodec::base32_rfc4648;

Utils::Utils() {}

// LEB128 encoding
std::vector<uint8_t> Utils::leb128_encode(uint64_t n) {
    std::vector<uint8_t> result;
    do {
        uint8_t byte = n & 0x7F;
        n >>= 7;
        if (n != 0) {
            byte |= 0x80;
        }
        result.push_back(byte);
    } while (n != 0);
    return result;
}


// SHA256
std::vector<uint8_t> Utils::sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(32);
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256, 1 for SHA-224
    mbedtls_sha256_update(&ctx, data.data(), data.size());
    mbedtls_sha256_finish(&ctx, hash.data());
    mbedtls_sha256_free(&ctx);
    return hash;
}

// Concatenate vectors
std::vector<uint8_t> Utils::concat(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Function to remove dashes from the cooked canister ID
std::string Utils::removeDashes(const std::string& input) {
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result), [](char c) { return c != '-'; });
    return result;
}

// Function to ensure the input string has correct padding for base32 decoding
std::string Utils::ensurePadding(const std::string& input) {
    std::string paddedInput = input;
    while (paddedInput.size() % 8 != 0) {
        paddedInput.push_back('=');
    }
    return paddedInput;
}

// Function to decode a base32 encoded string, ignoring dashes and ensuring padding
std::vector<uint8_t> Utils::unbase32(const std::string& input) {
    std::string cleanedInput = removeDashes(input);
    std::string paddedInput = ensurePadding(cleanedInput);
    std::vector<uint8_t> decoded = base32::decode(paddedInput);
    return decoded;
}

// Function to uncook a cooked canister ID
std::vector<uint8_t> Utils::uncook(const std::string& input) {
    std::vector<uint8_t> decoded = unbase32(input);
    // Discard the first 4 bytes (CRC)
    return std::vector<uint8_t>(decoded.begin() + 4, decoded.end());
}

// Function to encode raw bytes back to base32
std::string Utils::base32Encode(const std::vector<uint8_t>& input) {
    std::string encoded = base32::encode(input);
    
    // Insert dashes to match the format
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (i > 0 && i % 5 == 0) {
            result.push_back('-');
        }
        result.push_back(encoded[i]);
    }

    return result;
}

// Hash a CBOR structure
std::vector<uint8_t> Utils::cbor_hash(const std::string& value) {
    return sha256(std::vector<uint8_t>(value.begin(), value.end()));
}

std::vector<uint8_t> Utils::cbor_hash(const std::vector<uint8_t>& value) {
    return sha256(value);
}

std::vector<uint8_t> Utils::cbor_hash(uint64_t value) {
    return sha256(leb128_encode(value));
}

std::vector<uint8_t> Utils::cbor_hash(const std::vector<std::vector<uint8_t>>& array) {
    std::vector<uint8_t> concatenated;
    for (const auto& item : array) {
        auto hashed_item = cbor_hash(item);
        concatenated.insert(concatenated.end(), hashed_item.begin(), hashed_item.end());
    }
    return sha256(concatenated);
}

std::vector<uint8_t> Utils::cbor_hash(const std::map<std::vector<uint8_t>, std::vector<uint8_t>>& map) {
    std::vector<std::vector<uint8_t>> hash_pairs;
    for (const auto& [key, value] : map) {
        auto hashed_key = cbor_hash(key);
        //auto hashed_value = cbor_hash(value); Value is already hashed
        hash_pairs.push_back(concat(hashed_key, value));
    }
    std::sort(hash_pairs.begin(), hash_pairs.end());
    
    std::vector<uint8_t> concatenated;
    for (const auto& pair : hash_pairs) {
        concatenated.insert(concatenated.end(), pair.begin(), pair.end());
    }
    return sha256(concatenated);
}

std::vector<uint8_t> Utils::hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::vector<uint8_t> Utils::der_encode_signature(const std::vector<unsigned char>& signature) {
    // Split the signature into r and s
    size_t size = signature.size() / 2;
    std::vector<uint8_t> r(signature.begin(), signature.begin() + size);
    std::vector<uint8_t> s(signature.begin() + size, signature.end());

    std::vector<uint8_t> der;

    // DER encode the integers r and s
    der.push_back(0x02); // INTEGER
    der.push_back(r.size());
    der.insert(der.end(), r.begin(), r.end());

    der.push_back(0x02); // INTEGER
    der.push_back(s.size());
    der.insert(der.end(), s.begin(), s.end());

    // Wrap the encoded integers in a SEQUENCE
    std::vector<uint8_t> result;
    result.push_back(0x30); // SEQUENCE
    result.push_back(der.size());
    result.insert(result.end(), der.begin(), der.end());

    return result;
}

uint32_t Utils::crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = crc ^ byte;
        for (uint8_t j = 0; j < 8; j++) {
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }
    return ~crc;
}




