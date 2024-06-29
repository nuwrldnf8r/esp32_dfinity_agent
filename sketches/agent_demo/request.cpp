#include "request.h"
#include "utils.h"
#include <tinycbor.h>
#include <stdexcept>
#include <algorithm>
#include <ctime>
#include <vector>
#include <string>
#include <map>
#include "keypair.h"

uint8_t encode_buffer[1024];


uint64_t Request::calculateIngressExpiry(uint64_t durationInSeconds) const {
    time_t now = time(nullptr);
    return (static_cast<uint64_t>(now) + durationInSeconds) * 1000000000ULL;
}

std::vector<uint8_t> Request::stringToHexString(const std::string& input) const {
    const char hexDigits[] = "0123456789ABCDEF";
    size_t inputLength = input.size();
    size_t outputLength = inputLength * 2;
    std::vector<uint8_t> hexString(outputLength);

    for (size_t i = 0; i < inputLength; ++i) {
        unsigned char c = input[i];
        hexString[i * 2] = hexDigits[c >> 4];
        hexString[i * 2 + 1] = hexDigits[c & 0x0F];
    }

    return hexString;
}

std::vector<uint8_t> Request::hexStringToBytes(const std::string& hexString) const {
    size_t hexStringLength = hexString.size();
    size_t byteStringLength = hexStringLength / 2;
    std::vector<uint8_t> byteArray(byteStringLength);

    for (size_t i = 0; i < byteStringLength; ++i) {
        sscanf(hexString.c_str() + 2 * i, "%2hhx", &byteArray[i]);
    }

    return byteArray;
}

Request::Request() {}

Request::Request(const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args)
    : _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args) {
    _ingress_expiry = calculateIngressExpiry(60); // Set expiry time to 1 hour from now
}

Request::Request(const std::vector<uint8_t>& sender, const std::string& canisterId, const std::string& request_type, const std::string& method_name, const std::vector<uint8_t>& args)
    : _sender(sender), _canisterId(canisterId), _request_type(request_type), _method_name(method_name), _args(args) {
    _ingress_expiry = calculateIngressExpiry(60); // Set expiry time to 1 hour from now
}

std::vector<uint8_t> Request::encode(const Keypair& keypair) {
    // Sign the request
    sign(keypair);

    // Encode the request
    return encode();
}

std::vector<uint8_t> Request::encode() {
    int err = 0;

    // Initialize CBOR encoder
    CborEncoder encoder, mapEncoder, nestedMapEncoder;
    cbor_encoder_init(&encoder, encode_buffer, sizeof(encode_buffer), 0);

    // Convert canisterId to bytes using uncook
    std::vector<uint8_t> canister_id_bytes = Utils::uncook(_canisterId);

    // Convert sender to bytes, handling the case when _sender is empty
    std::vector<uint8_t> sender_bytes;
    if (_sender.empty()) {
        sender_bytes.push_back(0x04); // Use hex value 04
    } else {
        sender_bytes = std::vector<uint8_t>(_sender.begin(), _sender.end());
    }

    int num_fields = (_sender_pubkey.empty())?1:3;


    // Start CBOR encoding
    if ((err = cbor_encoder_create_map(&encoder, &mapEncoder, num_fields)) != CborNoError) throw std::runtime_error("Failed to create map");
    if ((err = cbor_encode_text_stringz(&mapEncoder, "content")) != CborNoError) throw std::runtime_error("Failed to encode content key");
    {
        if ((err = cbor_encoder_create_map(&mapEncoder, &nestedMapEncoder, 6)) != CborNoError) throw std::runtime_error("Failed to create nested map");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "ingress_expiry")) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry key");
        if ((err = cbor_encode_uint(&nestedMapEncoder, _ingress_expiry)) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "sender")) != CborNoError) throw std::runtime_error("Failed to encode sender key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, sender_bytes.data(), sender_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode sender value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "canister_id")) != CborNoError) throw std::runtime_error("Failed to encode canister_id key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, canister_id_bytes.data(), canister_id_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode canister_id value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "request_type")) != CborNoError) throw std::runtime_error("Failed to encode request_type key");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, _request_type.c_str())) != CborNoError) throw std::runtime_error("Failed to encode request_type value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "method_name")) != CborNoError) throw std::runtime_error("Failed to encode method_name key");
        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, _method_name.c_str())) != CborNoError) throw std::runtime_error("Failed to encode method_name value");

        if ((err = cbor_encode_text_stringz(&nestedMapEncoder, "arg")) != CborNoError) throw std::runtime_error("Failed to encode args key");
        if ((err = cbor_encode_byte_string(&nestedMapEncoder, _args.data(), _args.size())) != CborNoError) throw std::runtime_error("Failed to encode args value");
        
        if ((err = cbor_encoder_close_container(&mapEncoder, &nestedMapEncoder)) != CborNoError) throw std::runtime_error("Failed to close nested container");
    }
    //TODO: delegation

    printf("sender_pubkey: \n");
    for(auto byte : _sender_pubkey){
        printf("%02x", byte);
    }
    printf("\n");
    
    if(!_sender_pubkey.empty()){
        printf("not empty\n");
        //check for signature
        if(_sender_sig.empty()){
            throw std::runtime_error("Signature is required when sender_pubkey is provided");
        }
        if((err = cbor_encode_text_stringz(&mapEncoder, "sender_pubkey")) != CborNoError) throw std::runtime_error("Failed to encode sender_pubkey key");
        if((err = cbor_encode_byte_string(&mapEncoder, _sender_pubkey.data(), _sender_pubkey.size())) != CborNoError) throw std::runtime_error("Failed to encode sender_pubkey value");
        if((err = cbor_encode_text_stringz(&mapEncoder, "sender_sig")) != CborNoError) throw std::runtime_error("Failed to encode signature key");
        if((err = cbor_encode_byte_string(&mapEncoder, _sender_sig.data(), _sender_sig.size())) != CborNoError) throw std::runtime_error("Failed to encode sender_sig value");

    }
    

    if ((err = cbor_encoder_close_container(&encoder, &mapEncoder)) != CborNoError) throw std::runtime_error("Failed to close container");

    // Retrieve encoded data
    size_t sz = cbor_encoder_get_buffer_size(&encoder, encode_buffer);
    std::vector<uint8_t> result(encode_buffer, encode_buffer + sz);

    return result;
}

/*
std::vector<uint8_t> Request::createReadStateRequest(const std::string& canisterId, const std::vector<std::vector<std::string>>& paths) const {
    CborEncoder encoder, mapEncoder, contentMapEncoder, pathsArrayEncoder, pathArrayEncoder;
    cbor_encoder_init(&encoder, encode_buffer, sizeof(encode_buffer), 0);

    // Convert sender to bytes, handling the case when _sender is empty
    std::vector<uint8_t> sender_bytes;
    if (_sender.empty()) {
        sender_bytes.push_back(0x04); // Use hex value 04
    } else {
        sender_bytes = Utils::uncook(_sender);
    }

    

    int err;
    // Create the outermost map
    if ((err = cbor_encoder_create_map(&encoder, &mapEncoder, 1)) != CborNoError) throw std::runtime_error("Failed to create map");

    // Encode "content" key and its map value
    if ((err = cbor_encode_text_stringz(&mapEncoder, "content")) != CborNoError) throw std::runtime_error("Failed to encode content key");
    if ((err = cbor_encoder_create_map(&mapEncoder, &contentMapEncoder, 5)) != CborNoError) throw std::runtime_error("Failed to create nested map");

    // Encode "ingress_expiry"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "ingress_expiry")) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry key");
    if ((err = cbor_encode_uint(&contentMapEncoder, _ingress_expiry)) != CborNoError) throw std::runtime_error("Failed to encode ingress_expiry value");
    
    // Encode "canister_id"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "canister_id")) != CborNoError) throw std::runtime_error("Failed to encode canister_id key");
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, canisterId.c_str())) != CborNoError) throw std::runtime_error("Failed to encode canister_id value");

    //Encode "sender"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "sender")) != CborNoError) throw std::runtime_error("Failed to encode sender key");
    if ((err = cbor_encode_byte_string(&contentMapEncoder, sender_bytes.data(), sender_bytes.size())) != CborNoError) throw std::runtime_error("Failed to encode sender value");

    //Encode "request_type"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "request_type")) != CborNoError) throw std::runtime_error("Failed to encode request_type key");
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "read_state")) != CborNoError) throw std::runtime_error("Failed to encode request_type value");

    // Encode "paths"
    if ((err = cbor_encode_text_stringz(&contentMapEncoder, "paths")) != CborNoError) throw std::runtime_error("Failed to encode paths key");
    if ((err = cbor_encoder_create_array(&contentMapEncoder, &pathsArrayEncoder, paths.size())) != CborNoError) throw std::runtime_error("Failed to create paths array");

    for (const auto& path : paths) {
        if ((err = cbor_encoder_create_array(&pathsArrayEncoder, &pathArrayEncoder, path.size())) != CborNoError) throw std::runtime_error("Failed to create path array");
        for (const auto& segment : path) {
            // Encode each segment as a byte string (blob)
            if ((err = cbor_encode_byte_string(&pathArrayEncoder, reinterpret_cast<const uint8_t*>(segment.data()), segment.size())) != CborNoError) {
                throw std::runtime_error("Failed to encode path segment");
            }
        }
        if ((err = cbor_encoder_close_container(&pathsArrayEncoder, &pathArrayEncoder)) != CborNoError) throw std::runtime_error("Failed to close path array");
    }
    if ((err = cbor_encoder_close_container(&contentMapEncoder, &pathsArrayEncoder)) != CborNoError) throw std::runtime_error("Failed to close paths array");

    // Ensure additional keys and values in the nested map are correctly encoded
    if ((err = cbor_encoder_close_container(&mapEncoder, &contentMapEncoder)) != CborNoError) throw std::runtime_error("Failed to close nested map");
    if ((err = cbor_encoder_close_container(&encoder, &mapEncoder)) != CborNoError) throw std::runtime_error("Failed to close outermost map");

    size_t sz = cbor_encoder_get_buffer_size(&encoder, encode_buffer);
    std::vector<uint8_t> result(encode_buffer, encode_buffer + sz);

    return result;
}
*/

/*
std::vector<uint8_t> Request::generateRequestId() const {
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> content_map;
    std::vector<uint8_t> canister_id_bytes = Utils::uncook(_canisterId);
    // Fill content_map with the hashed key-value pairs of the content
    content_map[Utils::cbor_hash("ingress_expiry")] = Utils::cbor_hash(_ingress_expiry);
    content_map[Utils::cbor_hash("sender")] = Utils::cbor_hash(_sender.empty() ? std::vector<uint8_t>{0x04} : _sender);
    content_map[Utils::cbor_hash("canister_id")] = Utils::cbor_hash(canister_id_bytes);
    content_map[Utils::cbor_hash("request_type")] = Utils::cbor_hash(_request_type);
    content_map[Utils::cbor_hash("method_name")] = Utils::cbor_hash(_method_name);
    content_map[Utils::cbor_hash("arg")] = Utils::cbor_hash(_args);

    // Compute the hash of the entire content map
    std::vector<uint8_t> cbor_hashed = Utils::cbor_hash(content_map);

    // Append the prefix
    std::vector<uint8_t> prefixed_request = Utils::concat(std::vector<uint8_t>{0x0A, 'i', 'c', '-', 'r', 'e', 'q', 'u', 'e', 's', 't'}, cbor_hashed);

    // Compute the final hash
    return Utils::sha256(prefixed_request);
}
*/


std::vector<uint8_t> Request::generateRequestId() const {
    std::vector<std::vector<uint8_t>> hash_pairs;
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("ingress_expiry"), Utils::cbor_hash(_ingress_expiry)));
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("sender"), Utils::cbor_hash(_sender.empty() ? std::vector<uint8_t>{0x04} : _sender)));
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("canister_id"), Utils::cbor_hash(Utils::uncook(_canisterId))));
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("request_type"), Utils::cbor_hash(_request_type)));
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("method_name"), Utils::cbor_hash(_method_name)));
    hash_pairs.push_back(Utils::concat(Utils::cbor_hash("arg"), Utils ::cbor_hash(_args)));
    std::sort(hash_pairs.begin(), hash_pairs.end());
    std::vector<uint8_t> concatenated;
    for (const auto& pair : hash_pairs) {
        concatenated.insert(concatenated.end(), pair.begin(), pair.end());
    }
    auto hashed = Utils::sha256(concatenated);
    return hashed;
    std::vector<uint8_t> prefixed_request = Utils::concat(std::vector<uint8_t>{0x0A, 'i', 'c', '-', 'r', 'e', 'q', 'u', 'e', 's', 't'}, hashed);
    return prefixed_request;
}

void Request::sign(Keypair keypair) {
    printf("setting up for signing\n");
    //check that _sender is not empty
    if (_sender.empty()) {
        _sender = keypair.getPrincipal();
    }
    printf("setting pub key");

    _sender_pubkey = keypair.getPublicKey();
    if(!_sender_pubkey.empty()){
        printf("pub key set\n");
    }

    // Generate the request ID
    printf("generating request id\n");
    std::vector<uint8_t> requestId = generateRequestId();
    //add prefix
    std::vector<uint8_t> to_sign = Utils::concat(std::vector<uint8_t>{0x0A, 'i', 'c', '-', 'r', 'e', 'q', 'u', 'e', 's', 't'}, requestId);
    
    //to_sign = Utils::sha256(Utils::concat(to_sign, _sender_pubkey));
    to_sign = Utils::sha256(to_sign);


    //TODO - try sha

    // Generate the signature
    printf("signing\n");
    std::vector<uint8_t> signature = keypair.sign(to_sign);
    printf("signature:\n");
    for(auto byte : signature){
        printf("%02x", byte);
    }
    printf("\n");

    printf("actual public key:\n");    
    for(auto byte : keypair._public_key){
        printf("%02x", byte);
    }
    printf("\n");

    
    bool isVerified = keypair.verify(to_sign, keypair._public_key, signature);
    if(!isVerified){
        printf("not verified\n");
        throw std::runtime_error("Signature verification failed");
    }
    printf("verified\n");
    
    _sender_sig = signature; //Utils::der_encode_signature(signature);
    printf("signed\n");
}
