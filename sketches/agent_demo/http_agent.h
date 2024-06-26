#ifndef HTTPAGENT_H
#define HTTPAGENT_H

#include "keypair.h"
#include "request.h"
#include "candid.h"
#include <string>
#include <vector>
#include <cstdint>
#include <WiFiClientSecure.h>

class HttpAgent {
    public:
        HttpAgent();
        HttpAgent(const std::string& canisterID);
        HttpAgent(const std::string& canisterID, const Keypair& senderKeyPair);
        std::vector<Parameter> query(const std::string& method_name, const std::vector<Parameter>& args);
        std::string principal() const { return _senderKeyPair.getPrincipal(); } 
        std::vector<uint8_t> public_key() const { return _senderKeyPair.getPublicKey(); }
        
    private:
        std::string _canisterID;
        Keypair _senderKeyPair;
        WiFiClientSecure _client;
        bool _initialized = false;
};

#endif // HTTPAGENT_H