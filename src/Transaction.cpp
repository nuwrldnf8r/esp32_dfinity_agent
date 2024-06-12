#include "transaction.h"
#include <tinycbor.h>
#include "keypair.h"


#define CHECK_ERROR(proc) {\
  if( (err = proc) != 0) {\
    err_line = __LINE__;\
    goto on_error;\
  }\
}

uint8_t encode_buffer[1024];

std::vector<uint8_t> stringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}



Transaction::Transaction(const std::string& canisterId, const std::string& request_type, std::string& method_name, const std::string& args){
    _canisterId = canisterId;
    _request_type = request_type;
    _method_name = method_name;
    _args = args;
}

Transaction::Transaction(const std::string& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::string& args, const std::string& sender_pubkey){
    _sender = sender;
    _canisterId = canisterId;
    _request_type = request_type;
    _method_name = method_name;
    _args = args;
    _sender_pubkey = sender_pubkey;
}

std::vector<uint8_t> Transaction::encode(){
    bool isAnonymous = _sender.empty();
    if(isAnonymous) _sender = "04";
    TinyCBOR.init();
    TinyCBOR.Encoder.init(encode_buffer, sizeof(encode_buffer));
    
    uint8_t num_fields = 1;
    if(!_sender_pubkey.empty()) num_fields++;
    if(!_sender_sig.empty()) num_fields++;

    //TODO: also do for delegation
    // Start CBOR encoding //
    CHECK_ERROR(TinyCBOR.Encoder.create_map(num_fields));
    {CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("content"));
        CHECK_ERROR(TinyCBOR.Encoder.create_map(6));
        {
            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("ingress_expiry"));
            CHECK_ERROR(TinyCBOR.Encoder.encode_uint(ingress_expiry));   

            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("sender"));
            std::vector<uint8_t> sender_bytes = hexStringToBytes(_sender);
            CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(sender_bytes.data(), sender_bytes.size()));         

            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("canister_id"));
            std::vector<uint8_t> canister_id_bytes = hexStringToBytes(_canisterId);
            CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(canister_id_bytes.data(), canister_id_bytes.size()));

            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("request_type"));
            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz(_request_type.c_str()));

            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("method_name"));
            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz(_method_name.c_str()));

            CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("args"));
            std::vector<uint8_t> args_bytes = hexStringToBytes(_args);
            CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(args_bytes.data(), args_bytes.size()));

            std::vector<uint8_t> args_bytes = stringToBytes(args);
            CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(args_bytes.data(), args_bytes.size()));
        }
        CHECK_ERROR(TinyCBOR.Encoder.close_container());
    }
    if(!_sender_public_key.empty()){
        CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("sender_pubkey"));
        std::vector<uint8_t> sender_pubkey_bytes = hexStringToBytes(_sender_pubkey);
        CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(sender_pubkey_bytes.data(), sender_pubkey_bytes.size()));
    }
    if(!_sender_sig.empty()){
        CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("sender_sig"));
        std::vector<uint8_t> sender_sig_bytes = hexStringToBytes(_sender_sig);
        CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(sender_sig_bytes.data(), sender_sig_bytes.size()));
    }

    CHECK_ERROR(TinyCBOR.Encoder.close_container());
    // End CBOR encoding //
    size_t encoded_size = TinyCBOR.Encoder.get_encoded_size();

    // Create a vector and copy the encoded data into it.
    std::vector<uint8_t> encoded_data(encode_buffer, encode_buffer + encoded_size);

    return encoded_data;

}

std::string Transaction::encode_for_sig() {
    // Start with an empty vector of hashes
    std::vector<std::string> hashes;

    // For each field in the transaction data
    // Note: You'll need to replace 'transaction_data' with the actual data structure
    for (const auto& field : transaction_data) {
        // Hash the field's name
        std::string name_hash = hash(field.first);

        // Hash the field's value
        std::string value_hash = hash(field.second);

        // Concatenate the name hash and the value hash
        hashes.push_back(name_hash + value_hash);
    }

    // Sort the hashes
    std::sort(hashes.begin(), hashes.end());

    // Concatenate the sorted hashes and hash the result
    std::string final_hash = hash(std::accumulate(hashes.begin(), hashes.end(), std::string("")));

    return final_hash;
}

void Transaction::sign(const std::string& private_key){
    Keypair keypair(private_key);
    std::string signature = keypair.sign(encode());
    _sender_sig = signature;
}


