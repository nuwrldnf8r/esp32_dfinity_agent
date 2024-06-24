#ifndef HTTPAGENT_H
#define HTTPAGENT_H

#include "keypair.h"
#include "keypair.cpp"
#include "request.h"
#include "request.cpp"
#include <string>
#include <vector>
#include <cstdint>
#include <WiFiClientSecure.h>

class HttpAgent {
    public:
        HttpAgent();
        HttpAgent(const std::string& canisterID);
        HttpAgent(const std::string& canisterID, const Keypair& senderKeyPair);
        void query(const std::string& method_name, const std::vector<uint8_t>& args);
        std::string principal() const { return _senderKeyPair.getPrincipal(); } 
        std::vector<uint8_t> public_key() const { return _senderKeyPair.getPublicKey(); }
        
    private:
        std::string _canisterID;
        Keypair _senderKeyPair;
        WiFiClientSecure _client;
        bool _initialized = false;
};

#endif // HTTPAGENT_H
