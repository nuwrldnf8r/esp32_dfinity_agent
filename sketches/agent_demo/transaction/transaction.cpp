#include "transaction.h"
#include <tinycbor.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <ctime>

uint8_t encode_buffer[1024];

std::vector<uint8_t> unbase32(const std::string& input) {
    std::vector<uint8_t> result;
    uint32_t buffer = 0;
    int bitsLeft = 0;
    const std::string base32Chars = "abcdefghijklmnopqrstuvwxyz234567";

    for (char c : input) {
        auto pos = base32Chars.find(tolower(c));
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid character in base32 string");
        }
        buffer = (buffer << 5) | pos;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            result.push_back((buffer >> (bitsLeft - 8)) & 0xFF);
            bitsLeft -= 8;
        }
    }
    return result;
}

uint64_t calculateIngressExpiry(uint64_t durationInSeconds) {
    time_t now;
    time(&now);
    return now + durationInSeconds;
}


std::vector<uint8_t> Transaction::stringToHexString(const std::string& input) {
    const char hexDigits[] = "0123456789ABCDEF";
    size_t inputLength = input.size();
    size_t outputLength = inputLength * 2;
    std::vector<uint8_t> hexString(outputLength);

    for (size_t i = 0; i < inputLength; ++i) {
        unsigned char c = input[i];
        hexString[i * 2]     = hexDigits[c >> 4];
        hexString[i * 2 + 1] = hexDigits[c & 0x0F];
    }

    return hexString;
}

std::vector<uint8_t> Transaction::hexStringToBytes(const std::string& hexString) {
    size_t hexStringLength = hexString.size();
    size_t byteStringLength = hexStringLength / 2;
    std::vector<uint8_t> byteArray(byteStringLength);

    for (size_t i = 0; i < byteStringLength; ++i) {
        sscanf(hexString.c_str() + 2 * i, "%2hhx", &byteArray[i]);
    }

    return byteArray;
}

Transaction::Transaction(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args)
    : _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args) {
    _ingress_expiry = calculateIngressExpiry(3600); // Set expiry time to 1 hour from now
}

Transaction::Transaction(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args, const std::string& sender_pubkey)
    : _sender(sender), _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args), _sender_pubkey(sender_pubkey) {
    _ingress_expiry = calculateIngressExpiry(3600); // Set expiry time to 1 hour from now
}
std::vector<uint8_t> Transaction::encode() const {
    int err = 0;

    // Initialize TinyCBOR library.
    TinyCBOR.init();
    TinyCBOR.Encoder.init(encode_buffer, sizeof(encode_buffer));

    // Convert canisterId to bytes using unbase32
    std::vector<uint8_t> canister_id_bytes = unbase32(_canisterId);

    // Convert sender to bytes, handling the case when _sender is empty
    std::vector<uint8_t> sender_bytes;
    if (_sender.empty()) {
        sender_bytes.push_back(0x04); // Use hex value 04
    } else {
        sender_bytes = unbase32(_sender);
    }

    // Start CBOR encoding
    if ((err = TinyCBOR.Encoder.create_map(1)) != 0) throw std::runtime_error("Failed to create map");
    if ((err = TinyCBOR.Encoder.encode_text_stringz("content")) != 0) throw std::runtime_error("Failed to encode content key");
    {   
        if ((err = TinyCBOR.Encoder.create_map(6)) != 0) throw std::runtime_error("Failed to create nested map");
        if ((err = TinyCBOR.Encoder.encode_text_stringz("ingress_expiry")) != 0) throw std::runtime_error("Failed to encode ingress_expiry key");
        if ((err = TinyCBOR.Encoder.encode_uint(_ingress_expiry)) != 0) throw std::runtime_error("Failed to encode ingress_expiry value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("sender")) != 0) throw std::runtime_error("Failed to encode sender key");
        if ((err = TinyCBOR.Encoder.encode_byte_string(reinterpret_cast<const uint8_t*>(sender_bytes.data()), sender_bytes.size())) != 0) throw std::runtime_error("Failed to encode sender value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("canister_id")) != 0) throw std::runtime_error("Failed to encode canister_id key");
        if ((err = TinyCBOR.Encoder.encode_byte_string(reinterpret_cast<const uint8_t*>(canister_id_bytes.data()), canister_id_bytes.size())) != 0) throw std::runtime_error("Failed to encode canister_id value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("request_type")) != 0) throw std::runtime_error("Failed to encode request_type key");
        if ((err = TinyCBOR.Encoder.encode_text_stringz(_request_type.c_str())) != 0) throw std::runtime_error("Failed to encode request_type value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("method_name")) != 0) throw std::runtime_error("Failed to encode method_name key");
        if ((err = TinyCBOR.Encoder.encode_text_stringz(_method_name.c_str())) != 0) throw std::runtime_error("Failed to encode method_name value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("args")) != 0) throw std::runtime_error("Failed to encode args key");
        auto params = stringToHexString(_args);
        if ((err = TinyCBOR.Encoder.encode_byte_string(params.data(), params.size())) != 0) throw std::runtime_error("Failed to encode args value");

        if ((err = TinyCBOR.Encoder.close_container()) != 0) throw std::runtime_error("Failed to close nested container");
    }
    /*TODO: add pub key etc*/
    if ((err = TinyCBOR.Encoder.close_container()) != 0) throw std::runtime_error("Failed to close container");

    // Retrieve encoded data
    uint8_t* buf = TinyCBOR.Encoder.get_buffer();
    size_t sz = TinyCBOR.Encoder.get_buffer_size();
    std::vector<uint8_t> result(buf, buf + sz);

    return result;
}
