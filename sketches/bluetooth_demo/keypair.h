#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <vector>
#include <string>


class Keypair {
public:
    Keypair();
    //Keypair(const std::vector<unsigned char>& private_key_buf);
    void initialize();
    void initialize(bool from_storage);
    void initialize(const std::vector<unsigned char>& private_key_buf);
    std::vector<uint8_t> sign(const std::vector<unsigned char>& message);
    //bool verify(const std::vector<unsigned char>& message, const std::vector<uint8_t>& public_key, const std::vector<unsigned char>& signature);
    const std::vector<uint8_t>& getPrivateKey() const { return _private_key; }
    const std::vector<uint8_t>& getPublicKey() const { return _public_key_der; }
    bool isInitialized() const { return _is_initialized; }
    std::vector<uint8_t> getPrincipal() const;
    bool verify(const std::vector<unsigned char>& message, const std::vector<uint8_t>& public_key, const std::vector<unsigned char>& signature);
    std::vector<uint8_t> _public_key;
private:
    std::vector<uint8_t> _private_key;
    std::vector<uint8_t> _public_key_der;
    bool _is_initialized = false;
    std::vector<uint8_t> der_encode_public_key(const std::vector<uint8_t>& public_key);
    void generateKeypair();
};

#endif // KEYPAIR_H

