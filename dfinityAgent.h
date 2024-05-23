#ifndef dfinityAgent_h
#define dfinityAgent_h

#include <Arduino.h>

class dfinityAgent {
    public:
        uint8_t ever_le(uint64_t n, uint8_t* buffer);
        uint8_t biggie(size_t k, uint64_t n, uint8_t* buffer);
        uint64_t unbiggie(uint8_t* bytes, size_t len);
        uint8_t* cbor_tc(uint64_t ty, uint64_t n, size_t& length);
};

#endif
