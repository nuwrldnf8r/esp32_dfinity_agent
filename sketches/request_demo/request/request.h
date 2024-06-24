#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <cstdint>

class Request {
public:
    // Constructors
    Request(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args);
    Request(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args, const std::string& sender_pubkey);
    
    std::vector<uint8_t> encode() const;
    std::vector<uint8_t> createReadStateRequest(const std::string& canisterId, const std::vector<std::vector<std::string>>& paths) const; 
    std::string generateRequestId() const;

private:
    uint64_t _ingress_expiry;
    std::string _sender;
    std::string _sender_pubkey;
    std::string _canisterId;
    std::string _request_type;
    std::string _method_name;
    std::vector<uint8_t> _args;  // Changed to vector of bytes
    std::string _requestId;

    std::vector<uint8_t> stringToHexString(const std::string& input) const;
    std::vector<uint8_t> hexStringToBytes(const std::string& hexString) const;
    uint64_t calculateIngressExpiry(uint64_t durationInSeconds) const;
    std::vector<uint8_t> generateSHA256(const std::vector<uint8_t>& data) const;
};

#endif // TRANSACTION_H
