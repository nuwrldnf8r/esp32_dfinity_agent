#include "utils.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <mbedtls/sha256.h>
#include <cstdint>
#include <map>
#include <cppcodec/base32_rfc4648.hpp>

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
        auto hashed_value = cbor_hash(value);
        hash_pairs.push_back(concat(hashed_key, hashed_value));
    }
    std::sort(hash_pairs.begin(), hash_pairs.end());
    std::vector<uint8_t> concatenated;
    for (const auto& pair : hash_pairs) {
        concatenated.insert(concatenated.end(), pair.begin(), pair.end());
    }
    return sha256(concatenated);
}
