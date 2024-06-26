#include "request.h"
#include <tinycbor.h>
#include <stdexcept>
#include <algorithm>
#include <ctime>
#include <mbedtls/sha256.h>
#include <vector>
#include <mbedtls/sha256.h>
#include <string>
#include <map>
#include <cppcodec/base32_rfc4648.hpp>

uint8_t encode_buffer[1024];

bool isValidUTF8(const std::string &str) {
    int numBytes = 0;
    for (unsigned char c : str) {
        if (numBytes == 0) {
            if ((c >> 5) == 0x6) numBytes = 1;
            else if ((c >> 4) == 0xe) numBytes = 2;
            else if ((c >> 3) == 0x1e) numBytes = 3;
            else if ((c >> 7)) return false;
        } else {
            if ((c >> 6) != 0x2) return false;
            numBytes--;
        }
    }
    return numBytes == 0;
}

void ensureValidUTF8(const std::string &str) {
    if (!isValidUTF8(str)) {
        throw std::runtime_error("Invalid UTF-8 encoding in string: " + str);
    }
}

std::vector<uint8_t> hashPathSegment(const std::string& segment) {
    std::vector<uint8_t> hash(32); // SHA-256 outputs 32 bytes

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // Use 0 for SHA-256, 1 for SHA-224
    mbedtls_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(segment.data()), segment.size());
    mbedtls_sha256_finish(&ctx, hash.data());
    mbedtls_sha256_free(&ctx);

    return hash;
}

// LEB128 encoding
std::vector<uint8_t> leb128_encode(uint64_t n) {
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

//helper functions TODO - put in a seperate library
// Hash function using mbedtls
std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
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
std::vector<uint8_t> concat(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Hash a CBOR structure
std::vector<uint8_t> cbor_hash(const std::string& value) {
    return sha256(std::vector<uint8_t>(value.begin(), value.end()));
}

std::vector<uint8_t> cbor_hash(const std::vector<uint8_t>& value) {
    return sha256(value);
}

std::vector<uint8_t> cbor_hash(uint64_t value) {
    return sha256(leb128_encode(value));
}

std::vector<uint8_t> cbor_hash(const std::vector<std::vector<uint8_t>>& array) {
    std::vector<uint8_t> concatenated;
    for (const auto& item : array) {
        auto hashed_item = cbor_hash(item);
        concatenated.insert(concatenated.end(), hashed_item.begin(), hashed_item.end());
    }
    return sha256(concatenated);
}

std::vector<uint8_t> cbor_hash(const std::map<std::vector<uint8_t>, std::vector<uint8_t>>& map) {
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


using base32 = cppcodec::base32_rfc4648;

// Function to remove dashes from the cooked canister ID
std::string removeDashes(const std::string& input) {
    std::string result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result), [](char c) { return c != '-'; });
    return result;
}

// Function to ensure the input string has correct padding for base32 decoding
std::string ensurePadding(const std::string& input) {
    std::string paddedInput = input;
    while (paddedInput.size() % 8 != 0) {
        paddedInput.push_back('=');
    }
    return paddedInput;
}

// Function to decode a base32 encoded string, ignoring dashes and ensuring padding
std::vector<uint8_t> unbase32(const std::string& input) {
    std::string cleanedInput = removeDashes(input);
    std::string paddedInput = ensurePadding(cleanedInput);
    std::vector<uint8_t> decoded = base32::decode(paddedInput);
    return decoded;
}

// Function to uncook a cooked canister ID
std::vector<uint8_t> uncook(const std::string& input) {
    std::vector<uint8_t> decoded = unbase32(input);
    // Discard the first 4 bytes (CRC)
    return std::vector<uint8_t>(decoded.begin() + 4, decoded.end());
}

// Function to encode raw bytes back to base32
std::string base32Encode(const std::vector<uint8_t>& input) {
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




uint64_t Request::calculateIngressExpiry(uint64_t durationInSeconds) const {
    time_t now = time(nullptr);
    return (static_cast<uint64_t>(now) + durationInSeconds) * 1000000000ULL;
}

std::vector<uint8_t> Request::stringToHexString(const std::string& input) const {
    const char hexDigits[] = "0123456789ABCDEF";
    size_t inputLength = input.size();
    size_t outputLength = inputLength * 2;
    std::vector<uint8_t> hexString(outputLength);

    for (size_t i = 0; i < inputLength; ++i) {
        unsigned char c = input[i];
        hexString[i * 2] = hexDigits[c >> 4];
        hexString[i * 2 + 1] = hexDigits[c & 0x0F];
    }

    return hexString;
}

std::vector<uint8_t> Request::hexStringToBytes(const std::string& hexString) const {
    size_t hexStringLength = hexString.size();
    size_t byteStringLength = hexStringLength / 2;
    std::vector<uint8_t> byteArray(byteStringLength);

    for (size_t i = 0; i < byteStringLength; ++i) {
        sscanf(hexString.c_str() + 2 * i, "%2hhx", &byteArray[i]);
    }

    return byteArray;
}

Request::Request() {}

Request::Request(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args)
    : _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args) {
    _ingress_expiry = calculateIngressExpiry(60); // Set expiry time to 1 hour from now
}

Request::Request(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args, const std::vector<uint8_t>& sender_pubkey)
    : _sender(sender), _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args), _sender_pubkey(sender_pubkey) {
    _ingress_expiry = calculateIngressExpiry(60); // Set expiry time to 1 hour from now
}

std::vector<uint8_t> Request::encode() const {
    int err = 0;

    // Initialize CBOR encoder
    CborEncoder encoder, mapEncoder, nestedMapEncoder;
    cbor_encoder_init(&encoder, encode_buffer, sizeof(encode_buffer), 0);

    // Convert canisterId to bytes using uncook
    std::vector<uint8_t> canister_id_bytes = uncook(_canisterId);

    // Convert sender to bytes, handling the case when _sender is empty
    std::vector<uint8_t> sender_bytes;
    if (_sender.empty()) {
        sender_bytes.push_back(0x04); // Use hex value 04
    } else {
        sender_bytes = uncook(_sender);
    }

    // Start CBOR encoding
    if ((err = cbor_encoder_create_map(&encoder, &mapEncoder, 1)) != CborNoError) throw std::runtime_error("Failed to create map");
    if ((err = cbor_encode_text_stringz(&mapEncoder, "content")) != CborNoError) throw std::runtime_error("Failed to encode content key");
    {
        if ((err = cbor_encoder_create_map(&mapEncoder, &nestedMapEncoder, 6)) != CborNoError) throw std::runtime_error("Failed to create nested map");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "ingress_expiry")) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry key");
        if ((err = cbor_encode_uint(&nestedMapEncoder, _ingress_expiry)) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "sender")) != CborNoError) throw std::runtime_error("Failed to encode sender key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, sender_bytes.data(), sender_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode sender value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "canister_id")) != CborNoError) throw std::runtime_error("Failed to encode canister_id key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, canister_id_bytes.data(), canister_id_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode canister_id value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "request_type")) != CborNoError) throw std::runtime_error("Failed to encode request_type key");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, _request_type.c_str())) != CborNoError) throw std::runtime_error("Failed to encode request_type value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "method_name")) != CborNoError) throw std::runtime_error("Failed to encode method_name key");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, _method_name.c_str())) != CborNoError) throw std::runtime_error("Failed to encode method_name value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "arg")) != CborNoError) throw std::runtime_error("Failed to encode args key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, _args.data(), _args.size())) != CborNoError) throw std::runtime_error("Failed to encode args value");
        
        if ((err = cbor_encoder_close_container(&mapEncoder, &nestedMapEncoder)) != CborNoError) throw std::runtime_error("Failed to close nested container");
    }
    if ((err = cbor_encoder_close_container(&encoder, &mapEncoder)) != CborNoError) throw std::runtime_error("Failed to close container");

    // Retrieve encoded data
    size_t sz = cbor_encoder_get_buffer_size(&encoder, encode_buffer);
    std::vector<uint8_t> result(encode_buffer, encode_buffer + sz);

    return result;
}

std::vector<uint8_t> Request::createReadStateRequest(const std::string& canisterId, const std::vector<std::vector<std::string>>& paths) const {
    CborEncoder encoder, mapEncoder, contentMapEncoder, pathsArrayEncoder, pathArrayEncoder;
    cbor_encoder_init(&encoder, encode_buffer, sizeof(encode_buffer), 0);

    // Convert sender to bytes, handling the case when _sender is empty
    std::vector<uint8_t> sender_bytes;
    if (_sender.empty()) {
        sender_bytes.push_back(0x04); // Use hex value 04
    } else {
        sender_bytes = uncook(_sender);
    }

    

    int err;
    // Create the outermost map
    if ((err = cbor_encoder_create_map(&encoder, &mapEncoder, 1)) != CborNoError) throw std::runtime_error("Failed to create map");

    // Encode "content" key and its map value
    if ((err = cbor_encode_text_stringz(&mapEncoder, "content")) != CborNoError) throw std::runtime_error("Failed to encode content key");
    if ((err = cbor_encoder_create_map(&mapEncoder, &contentMapEncoder, 5)) != CborNoError) throw std::runtime_error("Failed to create nested map");

    // Encode "ingress_expiry"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "ingress_expiry")) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry key");
    if ((err = cbor_encode_uint(&contentMapEncoder, _ingress_expiry)) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry value");
    
    // Encode "canister_id"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "canister_id")) != CborNoError) throw std::runtime_error("Failed to encode canister_id key");
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, canisterId.c_str())) != CborNoError) throw std::runtime_error("Failed to encode canister_id value");

    //Encode "sender"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "sender")) != CborNoError) throw std::runtime_error("Failed to encode sender key");
    if ((err = cbor_encode_byte_string(&contentMapEncoder, sender_bytes.data(), sender_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode sender value");

    //Encode "request_type"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "request_type")) != CborNoError) throw std::runtime_error("Failed to encode request_type key");
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "read_state")) != CborNoError) throw std::runtime_error("Failed to encode request_type value");

    // Encode "paths"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "paths")) != CborNoError) throw std::runtime_error("Failed to encode paths key");
    if ((err = cbor_encoder_create_array(&contentMapEncoder, &pathsArrayEncoder, paths.size())) != CborNoError) throw std::runtime_error("Failed to create paths array");

    for (const auto& path : paths) {
        if ((err = cbor_encoder_create_array(&pathsArrayEncoder, &pathArrayEncoder, path.size())) != CborNoError) throw std::runtime_error("Failed to create path array");
        for (const auto& segment : path) {
            // Encode each segment as a byte string (blob)
            if ((err = cbor_encode_byte_string(&pathArrayEncoder, reinterpret_cast<const uint8_t*>(segment.data()), segment.size())) != CborNoError) {
                throw std::runtime_error("Failed to encode path segment");
            }
        }
        if ((err = cbor_encoder_close_container(&pathsArrayEncoder, &pathArrayEncoder)) != CborNoError) throw std::runtime_error("Failed to close path array");
    }
    if ((err = cbor_encoder_close_container(&contentMapEncoder, &pathsArrayEncoder)) != CborNoError) throw std::runtime_error("Failed to close paths array");

    // Ensure additional keys and values in the nested map are correctly encoded
    if ((err = cbor_encoder_close_container(&mapEncoder, &contentMapEncoder)) != CborNoError) throw std::runtime_error("Failed to close nested map");
    if ((err = cbor_encoder_close_container(&encoder, &mapEncoder)) != CborNoError) throw std::runtime_error("Failed to close outermost map");

    size_t sz = cbor_encoder_get_buffer_size(&encoder, encode_buffer);
    std::vector<uint8_t> result(encode_buffer, encode_buffer + sz);

    return result;
}

std::vector<uint8_t> Request::generateSHA256(const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> hash(32); // SHA-256 outputs 32 bytes

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // Use 0 for SHA-256, 1 for SHA-224
    mbedtls_sha256_update(&ctx, data.data(), data.size());
    mbedtls_sha256_finish(&ctx, hash.data());
    mbedtls_sha256_free(&ctx);

    return hash;
}

std::string Request::generateRequestId() const {
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> content_map;

    // Fill content_map with the hashed key-value pairs of the content
    content_map[cbor_hash("ingress_expiry")] = cbor_hash(_ingress_expiry);
    content_map[cbor_hash("sender")] = cbor_hash(_sender.empty() ? std::vector<uint8_t>{0x04} : uncook(_sender));
    content_map[cbor_hash("canister_id")] = cbor_hash(_canisterId);
    content_map[cbor_hash("request_type")] = cbor_hash(_request_type);
    content_map[cbor_hash("method_name")] = cbor_hash(_method_name);
    content_map[cbor_hash("arg")] = cbor_hash(_args);

    // Compute the hash of the entire content map
    auto request_id_hash = cbor_hash(content_map);

    // Convert the hash to a hex string
    std::string request_id(request_id_hash.begin(), request_id_hash.end());
    return request_id;
}

