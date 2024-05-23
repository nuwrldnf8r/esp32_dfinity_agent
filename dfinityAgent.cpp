#include "dfinityAgent.h"

#define MAX_BYTES 8
#define MAX_CBOR_SIZE 9

uint8_t dfinityAgent::ever_le(uint64_t n, uint8_t* buffer) {
    uint8_t numBytes = 0;
    do {
        buffer[numBytes++] = n % 256;
        n /= 256;
    } while (n > 0 && numBytes < MAX_BYTES);
    return numBytes;
}

uint8_t dfinityAgent::biggie(size_t k, uint64_t n, uint8_t* buffer) {
    uint8_t numBytes = ever_le(n, buffer);

    /*
    // Pad with zeros if necessary
    for (int i = numBytes; i < k; ++i) {
        buffer[i] = 0;
    }
    */
    numBytes = min(numBytes, static_cast<uint8_t>(k));

    // Reverse the bytes for big-endian
    for (int i = 0; i < k / 2; ++i) {
        uint8_t temp = buffer[i];
        buffer[i] = buffer[k - 1 - i];
        buffer[k - 1 - i] = temp;
    }

     

    return numBytes;
}


uint64_t dfinityAgent::unbiggie(uint8_t* bytes, size_t len) {
    uint64_t acc = 0;
    for (size_t i = 0; i < len; ++i) {
        acc = 256 * acc + bytes[i];
    }
    return acc;
}

uint8_t* dfinityAgent::cbor_tc(uint64_t ty, uint64_t n, size_t& length) {
    uint8_t* buffer = new uint8_t[MAX_CBOR_SIZE];
    if (n < 24) {
        buffer[0] = ty * 32 + n;
        length = 1; // Length is 1 byte
    } else if (n < 256) {
        buffer[0] = ty * 32 + 24;
        buffer[1] = n & 0xFF; // Store the least significant byte of n
        length = 2; // Length is 2 bytes
    } else if (n < 65536) {
        buffer[0] = ty * 32 + 25;
        length = ever_le(n, &buffer[1]) + 1; // Length includes the type byte
    } else if (n < 4294967296) {
        buffer[0] = ty * 32 + 26;
        length = biggie(4, n, &buffer[1]) + 1; // Length includes the type byte
    } else {
        buffer[0] = ty * 32 + 27;
        length = biggie(8, n, &buffer[1]) + 1; // Length includes the type byte
    }
    return buffer; 
}



