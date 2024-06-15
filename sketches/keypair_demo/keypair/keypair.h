#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <vector>


class Keypair {
public:
    Keypair();
    //Keypair(const std::vector<unsigned char>& private_key_buf);
    void initialize();
    void initialize(bool from_storage);
    void initialize(const std::vector<unsigned char>& private_key_buf);
    std::vector<unsigned char> sign(const std::vector<unsigned char>& message);
    bool verify(const std::vector<unsigned char>& message, const std::vector<uint8_t>& public_key, const std::vector<unsigned char>& signature);
    const std::vector<uint8_t>& getPrivateKey() const { return _private_key; }
    const std::vector<uint8_t>& getPublicKey() const { return _public_key; }
    bool isInitialized() const { return _is_initialized; }

private:
    std::vector<uint8_t> _private_key;
    std::vector<uint8_t> _public_key;
    bool _is_initialized = false;
};

#endif // KEYPAIR_H

