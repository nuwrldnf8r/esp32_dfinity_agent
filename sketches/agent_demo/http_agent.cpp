#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <string>
#include <vector>
#include <tinycbor.h>
#include <cstdint>
#include "keypair.h"
#include "request.h"
#include "http_agent.h"
#include "candid.h"

//Helper functions:
const String baseURL = "https://ic0.app/api/v2/canister/";
const int httpsPort = 443;

String urlEncode(const String &value) {
    String encoded;
    char hex[4];
    for (int i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

class Signature {
public:
    uint64_t timestamp;
    std::string signature;
    std::string identity;
};

class Reply {
public:
    std::vector<uint8_t> arg;
};

class Response {
public:
    std::string status;
    Reply reply;
    std::vector<Signature> signatures;
};

HttpAgent::HttpAgent() {}

HttpAgent::HttpAgent(const std::string& canisterID) : _canisterID(canisterID) {
    _initialized = true;
}

HttpAgent::HttpAgent(const std::string& canisterID, const Keypair& senderKeyPair) : _canisterID(canisterID), _senderKeyPair(senderKeyPair) {
    _initialized = true;
}  

Response decodeCBOR(const std::vector<uint8_t>& data) {

    CborParser parser;
    CborValue it;

    CborError err = cbor_parser_init(data.data(), data.size(), 0, &parser, &it);
    if (err != CborNoError) {
        // Handle the error
    }

    // If the current item is a tag, skip over it
    if (cbor_value_get_type(&it) == CborTagType) {
        err = cbor_value_advance(&it);
        if (err != CborNoError) {
            // Handle the error
        }
    }

    Response response;
    CborValue map;

    err = cbor_value_enter_container(&it, &map);
    if (err != CborNoError) {
        // Handle the error
    }

    while (!cbor_value_at_end(&map)) {
        char* key;
        size_t keylen;
        err = cbor_value_dup_text_string(&map, &key, &keylen, NULL);
        if (err != CborNoError) {
            // Handle the error
        }

        err = cbor_value_advance(&map);
        if (err != CborNoError) {
            // Handle the error
        }

        if (strcmp(key, "status") == 0) {
            char* status;
            size_t statuslen;
            err = cbor_value_dup_text_string(&map, &status, &statuslen, NULL);
            if (err != CborNoError) {
                // Handle the error
            }
            response.status = status;
            free(status);

        } else if (strcmp(key, "reply") == 0) {
            
            if (cbor_value_get_type(&map) == CborMapType) {
                CborValue reply_map;
                printf("Entering map\n");
                err = cbor_value_enter_container(&map, &reply_map);
                if (err != CborNoError) {
                    // Handle the error
                    printf("Error entering map - 120\n");
                }
                printf("Entered map\n");

                while (!cbor_value_at_end(&reply_map)) {
                    char* reply_key;
                    size_t reply_keylen;
                    err = cbor_value_dup_text_string(&reply_map, &reply_key, &reply_keylen, NULL);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error getting key - 129\n");
                    }

                    err = cbor_value_advance(&reply_map);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error advancing - 134\n");
                    }

                    if (strcmp(reply_key, "arg") == 0) {
                        printf("getting arg\n");
                        if (cbor_value_get_type(&reply_map) == CborByteStringType) {
                            size_t arglen;
                            err = cbor_value_calculate_string_length(&reply_map, &arglen);
                            if (err != CborNoError) {
                                // Handle the error
                                printf("Error calculating length - 138\n");
                            }
                            std::vector<uint8_t> arg(arglen);
                            err = cbor_value_copy_byte_string(&reply_map, arg.data(), &arglen, NULL);
                            if (err != CborNoError) {
                                // Handle the error
                                printf("Error getting arg - 145\n");
                            }
                            response.reply.arg = std::move(arg);
                        } else {
                            // Handle the error
                            printf("Error: expected a byte string\n");
                        }
                    }

                    free(reply_key);
                    err = cbor_value_advance(&reply_map);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error advancing - 149\n");
                    }
                }
                
            } else {
                // Handle the error
                printf("Error getting type - 147\n");
            }
        } else if (strcmp(key, "signatures") == 0) {
            // Handle the "signatures" field
        }

        free(key);
        err = cbor_value_advance(&map);
        if (err != CborNoError) {
            // Handle the error
        }
    }

    return response;
}

void HttpAgent::query(const std::string& method_name, const std::vector<Parameter>& args) {
    if(!_initialized) {
        throw std::runtime_error("HttpAgent not initialized");
    }
    try{
        Request request;
        Candid candid(args);
        std::vector<uint8_t> _args = candid.encode();
        if(!_senderKeyPair.isInitialized()){
            request = Request(_canisterID, "query", method_name, _args);
        } else {
            request = Request(_senderKeyPair.getPrincipal(), _canisterID, "query", method_name, _args, _senderKeyPair.getPublicKey());
        }
        std::vector<uint8_t> encoded = request.encode();
        
        for (auto byte : encoded) {
            printf("%02x", byte);
        }
        printf("\n");

        String encodedCanisterId = urlEncode(_canisterID.c_str());
        String url = baseURL + encodedCanisterId + "/query";
        //for now/////////////////////////////////////
        _client.setInsecure();
        /////////////////////////////////////////////
        HTTPClient http;
        http.begin(_client, url);
        http.addHeader("Content-Type", "application/cbor");
        //http.addHeader("X-Canister-Authz", "Bearer " + _senderKeyPair.getPrincipal()); 
        
        printf("Connecting to ");
        printf(url.c_str());
        printf("\n");

        int httpResponseCode = http.POST(encoded.data(), encoded.size()); 
        if (httpResponseCode > 0) {
            printf("Response code: \n%d", httpResponseCode);
            printf("\n");
            printf("Response: \n");

            String response = http.getString();
            printf(response.c_str());
            printf("\n");

            // Assuming you get a CBOR response and want to print it as hex
            std::vector<uint8_t> responseBytes(response.begin(), response.end());
            Response r = decodeCBOR(responseBytes);
            printf("Decoded response: \n");
            printf("Status: ");
            printf(r.status.c_str());
            printf("\n");
            printf("Reply: ");
            for (auto byte : r.reply.arg) {
                printf("%02x", byte);
            }
            printf("\n");
            printf("***********************************\n");
            
            //TODO: CBOR decode
            
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
            Serial.print("Error message: ");
            Serial.println(http.errorToString(httpResponseCode).c_str());
        }
        http.end();      

    } catch(const std::exception& e) {
        Serial.println(e.what());
    }

}


/*
    std::string requestId = _request.generateRequestId();
    Request request(_senderKeyPair.getPrincipal(), _canisterID, "call", method_name, args, _senderKeyPair.getPublicKey());
    std::vector<std::vector<std::string>> paths = {{"request_status", requestId, "reply"}};
    std::vector<uint8_t> readStateRequest = request.createReadStateRequest(_canisterID, paths);
    std::vector<uint8_t> signedRequest = _senderKeyPair.sign(readStateRequest);
    std::vector<uint8_t> encodedRequest = request.encode();
    std::string encodedRequestStr(encodedRequest.begin(), encodedRequest.end());
    std::string signedRequestStr(signedRequest.begin(), signedRequest.end());
    std::string host = "https://ic0.app/api/v2/canister/" + _canisterID + "/query";
    HTTPClient http;
    http.begin(client, host);
    http.addHeader("Content-Type", "application/cbor");
    http.addHeader("X-Canister-Authz", "Bearer " + _senderKeyPair.getPrincipal());
    http.addHeader("X-Canister-Date", "2021-08-25T00:00:00Z");
    http.addHeader("X-Canister-Sig", signedRequestStr);
    int httpResponseCode = http.POST(encodedRequestStr);
    if (httpResponseCode > 0) {
        String payload = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(payload);
    } else {
        Serial.println("Error on sending POST request");
    }
    http.end();
    */