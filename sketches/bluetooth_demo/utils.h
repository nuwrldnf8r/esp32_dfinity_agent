#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include "keypair.h"

class Utils{
    public:
        Utils();
        static std::vector<uint8_t> leb128_encode(uint64_t n);
        static std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
        static std::vector<uint8_t> concat(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);
        static std::string removeDashes(const std::string& input);
        static std::string ensurePadding(const std::string& input);
        static std::string base32Encode(const std::vector<uint8_t>& input);
        static std::vector<uint8_t> unbase32(const std::string& input);
        static std::vector<uint8_t> uncook(const std::string& input);
        static std::vector<uint8_t> cbor_hash(const std::string& value);
        static std::vector<uint8_t> cbor_hash(const std::vector<uint8_t>& value);
        static std::vector<uint8_t> cbor_hash(uint64_t value);
        static std::vector<uint8_t> cbor_hash(const std::vector<std::vector<uint8_t>>& array);
        static std::vector<uint8_t> cbor_hash(const std::map<std::vector<uint8_t>, std::vector<uint8_t>>& map);
        static std::vector<uint8_t> der_encode_signature(const std::vector<unsigned char>& signature);
        static uint32_t crc32(const uint8_t *data, size_t length);
        static std::vector<uint8_t> hex_to_bytes(const std::string& hex);
        static std::string bytes_to_hex(const std::vector<uint8_t>& bytes);
        
        
};
#endif // UTILS_H