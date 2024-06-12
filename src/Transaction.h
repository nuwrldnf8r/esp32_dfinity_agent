#ifndef TRANSACTION_H
#define TRANSACTION_H

class Transaction {
public:
    // Constructors
    Transaction(const std::string& canisterId, const std::string& request_type, std::string& method_name, const std::string& args);
    Transaction(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args, const std::string& _sender_pubkey);
    
    std::vector<uint8_t> encode();
    //void addDelegation(const std::string& pubkey, const std::string& delegation);
    void sign(const std::string& private_key); 

private:
    uint64_t _ingress_expiry, 
    std::string _sender;
    std::string _sender_pubkey;
    std::string _canisterId;
    std::string _request_type;
    std::string _method_name;
    std::string _args;
    std::string _sender_sig;
    std::vector<std::map<std::string, std::string>> sender_delegation;
    std::string Transaction::encode_for_sig();
};

#endif // TRANSACTION_H