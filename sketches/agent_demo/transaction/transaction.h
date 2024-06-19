#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>
#include <cstdint>

class Transaction {
public:
    // Constructors
    Transaction(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args);
    Transaction(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args, const std::string& sender_pubkey);

    std::vector<uint8_t> encode() const;

private:
    uint64_t _ingress_expiry;
    std::string _sender;
    std::string _sender_pubkey;
    std::string _canisterId;
    std::string _request_type;
    std::string _method_name;
    std::string _args;
    std::string _sender_sig;

    static std::vector<uint8_t> stringToHexString(const std::string& input);
    static std::vector<uint8_t> hexStringToBytes(const std::string& hexString);
};

#endif // TRANSACTION_H
