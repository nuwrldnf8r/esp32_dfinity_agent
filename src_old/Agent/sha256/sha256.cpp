#include "sha256.h"

// SHA-256 constants
static const uint32_t k[64] = {
    // (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311):
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    // (next 32 entries)...
    0x5807aa98, 0x72a6787d, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6,
    0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8,
    0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70,
    0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c,
    0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814,
    0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

SHA256::SHA256() {
    init();
}

void SHA256::init() {
    bitCount = 0;
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
}

void SHA256::update(const uint8_t *data, size_t len) {
    size_t i = 0;

    // Update the bit count
    bitCount += len * 8;

    // Process each byte
    while (len--) {
        buffer[bitCount / 8 % SHA256_BLOCK_SIZE] = *data++;
        if (bitCount / 8 % SHA256_BLOCK_SIZE == 0) {
            transform(buffer);
        }
    }
}

void SHA256::final(uint8_t *hash) {
    // Pad the message
    uint8_t pad[SHA256_BLOCK_SIZE];
    memset(pad, 0, SHA256_BLOCK_SIZE);
    pad[0] = 0x80;

    size_t padLen = (bitCount % SHA256_BLOCK_SIZE < 56) ? (56 - bitCount % SHA256_BLOCK_SIZE) : (120 - bitCount % SHA256_BLOCK_SIZE);
    update(pad, padLen);

    // Append the bit count
    for (int i = 0; i < 8; ++i) {
        buffer[63 - i] = (bitCount >> (8 * i)) & 0xff;
    }
    transform(buffer);

    // Output the hash
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            *hash++ = (state[i] >> (24 - j * 8)) & 0xff;
        }
    }
}

void SHA256::transform(const uint8_t *data) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t w[64];
    uint32_t t1, t2;

    // Prepare the message schedule
    for (int i = 0; i < 16; ++i) {
        w[i] = (data[4 * i] << 24) | (data[4 * i + 1] << 16) | (data[4 * i + 2] << 8) | (data[4 * i + 3]);
    }

    for (int i = 16; i < 64; ++i) {
        w[i] = w[i - 16] + (ROTRIGHT(w[i - 15], 7) ^ ROTRIGHT(w[i - 15], 18) ^ (w[i - 15] >> 3)) + w[i - 7] + (ROTRIGHT(w[i - 2], 17) ^ ROTRIGHT(w[i - 2], 19) ^ (w[i - 2] >> 10));
    }

    // Initialize the working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    // Perform the main hash computation
    for (int i = 0; i < 64; ++i) {
        t1 = h + (ROTRIGHT(e, 6) ^ ROTRIGHT(e, 11) ^ ROTRIGHT(e, 25)) + ((e & f) ^ ((~e) & g)) + k[i] + w[i];
        t2 = (ROTRIGHT(a, 2) ^ ROTRIGHT(a, 13) ^ ROTRIGHT(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Add the working variables back to the state
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

#define ROTRIGHT(x, y) (((x) >> (y)) | ((x) << (32 - (y))))
