#include "transaction.h"
#include <tinycbor.h>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cstring>

uint8_t encode_buffer[1024];

std::vector<uint8_t> hexStringToBytes(const std::string& hexString) {
    size_t hexStringLength = hexString.length();
    size_t byteStringLength = hexStringLength / 2;
    std::vector<uint8_t> byteArray(byteStringLength);

    for (size_t i = 0; i < byteStringLength; ++i) {
        sscanf(hexString.c_str() + 2 * i, "%2hhx", &byteArray[i]);
    }

    return byteArray;
}

std::vector<uint8_t> stringToHexString(const std::string& input) {
    const char hexDigits[] = "0123456789ABCDEF";
    size_t inputLength = input.length();
    size_t outputLength = inputLength * 2;
    std::vector<uint8_t> hexString(outputLength);

    for (size_t i = 0; i < inputLength; ++i) {
        unsigned char c = input[i];
        hexString[i * 2]     = hexDigits[c >> 4];
        hexString[i * 2 + 1] = hexDigits[c & 0x0F];
    }

    return hexString;
}

Transaction::Transaction(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args)
    : _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args), _ingress_expiry(0) {}

Transaction::Transaction(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args, const std::string& sender_pubkey)
    : _sender(sender), _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args), _sender_pubkey(sender_pubkey), _ingress_expiry(0) {}

std::vector<uint8_t> Transaction::encode() const {
    int err = 0;

    // Initialize TinyCBOR library.
    TinyCBOR.init();
    TinyCBOR.Encoder.init(encode_buffer, sizeof(encode_buffer));

    // Start CBOR encoding
    if ((err = TinyCBOR.Encoder.create_map(1)) != 0) throw std::runtime_error("Failed to create map");
    if ((err = TinyCBOR.Encoder.encode_text_stringz("content")) != 0) throw std::runtime_error("Failed to encode content key");
    {   
        if ((err = TinyCBOR.Encoder.create_map(6)) != 0) throw std::runtime_error("Failed to create nested map");
        if ((err = TinyCBOR.Encoder.encode_text_stringz("ingress_expiry")) != 0) throw std::runtime_error("Failed to encode ingress_expiry key");
        if ((err = TinyCBOR.Encoder.encode_uint(_ingress_expiry)) != 0) throw std::runtime_error("Failed to encode ingress_expiry value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("sender")) != 0) throw std::runtime_error("Failed to encode sender key");
        auto sender_bytes = hexStringToBytes(_sender);
        if ((err = TinyCBOR.Encoder.encode_byte_string(sender_bytes.data(), sender_bytes.size())) != 0) throw std::runtime_error("Failed to encode sender value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("canister_id")) != 0) throw std::runtime_error("Failed to encode canister_id key");
        auto canister_id_bytes = hexStringToBytes(_canisterId);
        if ((err = TinyCBOR.Encoder.encode_byte_string(canister_id_bytes.data(), canister_id_bytes.size())) != 0) throw std::runtime_error("Failed to encode canister_id value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("request_type")) != 0) throw std::runtime_error("Failed to encode request_type key");
        if ((err = TinyCBOR.Encoder.encode_text_stringz(_request_type.c_str())) != 0) throw std::runtime_error("Failed to encode request_type value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("method_name")) != 0) throw std::runtime_error("Failed to encode method_name key");
        if ((err = TinyCBOR.Encoder.encode_text_stringz(_method_name.c_str())) != 0) throw std::runtime_error("Failed to encode method_name value");

        if ((err = TinyCBOR.Encoder.encode_text_stringz("args")) != 0) throw std::runtime_error("Failed to encode args key");
        auto params = stringToHexString(_args);
        if ((err = TinyCBOR.Encoder.encode_byte_string(params.data(), params.size())) != 0) throw std::runtime_error("Failed to encode args value");

        if ((err = TinyCBOR.Encoder.close_container()) != 0) throw std::runtime_error("Failed to close nested container");
    }
    if (!_sender_pubkey.empty()) {
        if ((err = TinyCBOR.
