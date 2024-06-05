#ifndef SHA256_H
#define SHA256_H

#include <Arduino.h>

#define SHA256_BLOCK_SIZE 64 // Block size in bytes
#define SHA256_HASH_SIZE 32  // Hash size in bytes

class SHA256 {
public:
    SHA256();
    void init();
    void update(const uint8_t *data, size_t length);
    void final(uint8_t *hash);

private:
    void transform(const uint8_t *data);
    uint32_t state[8];
    uint64_t bitCount;
    uint8_t buffer[SHA256_BLOCK_SIZE];
};

#endif // SHA256_H
