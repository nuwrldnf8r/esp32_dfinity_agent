#ifndef KEYPAIR_H
#define KEYPAIR_H

class Keypair {
public:
    Keypair();
    Keypair(const std::string& private_random_number);
    std::string getRandom();
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generate(const std::string& private_random_number); 
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generate(const std::vector<uint8_t>& private_key_vec);
    void keypair::generate(const std::string& private_random_number);
    std::vector<uint8_t> getPublicKey();
    std::vector<uint8_t> getPrivateKey();
    std::string getPublicKeyString();
    std::string getPrivateKeyString();
    std::vector<uint8_t> sign(const std::vector<uint8_t>& message);
    bool verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature);
    std::string sign(const std::string& message);
    bool verify(const std::string& message, const std::string& signature);

private:
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> _keypair;
};

#endif // KEYPAIR_H

