//#include <HTTPClient.h>
//#include <WiFi.h>
#include <string>
#include <vector>
#include "response.h"
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


HttpAgent::HttpAgent() {}

HttpAgent::HttpAgent(const std::string& canisterID) : _canisterID(canisterID) {
    _initialized = true;
}

HttpAgent::HttpAgent(const std::string& canisterID, const Keypair& senderKeyPair) : _canisterID(canisterID), _senderKeyPair(senderKeyPair) {
    _initialized = true;
}  

std::vector<Parameter> HttpAgent::post(const std::string& method, const std::string& method_name, const std::vector<Parameter>& args){
    std::vector<Parameter> result;
    if(!_initialized) {
        throw std::runtime_error("HttpAgent not initialized");
    }
    
    Request request;
    Candid candid(args);
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> _args = candid.encode();
    if(!_senderKeyPair.isInitialized()){
        request = Request(_canisterID, method, method_name, _args);
        encoded = request.encode();
    } else {
        request = Request(_senderKeyPair.getPrincipal(), _canisterID, method, method_name, _args);
        encoded = request.encode(_senderKeyPair);
    }

    String encodedCanisterId = urlEncode(_canisterID.c_str());
    String url = baseURL + encodedCanisterId + "/" + method.c_str();
    //for now/////////////////////////////////////
    //_client.setInsecure();
    /////////////////////////////////////////////
    _client.begin(url);
    _client.addHeader("Content-Type", "application/cbor");
    //http.addHeader("X-Canister-Authz", "Bearer " + _senderKeyPair.getPrincipal()); 
    
    printf("Connecting to ");
    printf(url.c_str());
    printf("\n");

    int httpResponseCode = _client.POST(encoded.data(), encoded.size()); 
    if (httpResponseCode > 0) {
        printf("Response code: \n%d", httpResponseCode);
        printf("\n");
        
        String response = _client.getString();

        // Assuming you get a CBOR response and want to print it as hex
        std::vector<uint8_t> responseBytes(response.begin(), response.end());
        for(auto byte : responseBytes) {
            printf("%02x", byte);
        }
        printf("\n");

        if(httpResponseCode == 200) {
            Response r = Response(responseBytes);
            printf("Received:\n");
            for(auto byte : r.reply.arg) {
                printf("%02x", byte);
            }   
            printf("\n");
            if(r.reply.arg.size() == 0) {
                printf("Empty response\n");
                result = {};
            } else {
                Candid candidResponse(r.reply.arg);
                result = candidResponse.decode();
            }
        } else if(httpResponseCode == 202){
            //manage call request

        } else {
            //Response r = Response(responseBytes);
            printf("Error on sending POST: ");
            printf("%d", httpResponseCode);
            printf("\n");
            printf("Error message: \n");
            printf("%s\n", _client.errorToString(httpResponseCode).c_str()); // Print the error message here
            printf("\n");
            return {};
        }
        
        
    } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        Serial.print("Error message: ");
        Serial.println(_client.errorToString(httpResponseCode).c_str());
        throw std::runtime_error("Error on sending POST request");
    }
    _client.end();      

    return result;
}

std::vector<Parameter> HttpAgent::update(const std::string& method_name, const std::vector<Parameter>& args) {
    return post("call", method_name, args);
}

std::vector<Parameter> HttpAgent::query(const std::string& method_name, const std::vector<Parameter>& args) {
    return post("query", method_name, args);
}





/*
std::vector<Parameter> HttpAgent::query(const std::string& method_name, const std::vector<Parameter>& args) {
    std::vector<Parameter> result;
    if(!_initialized) {
        throw std::runtime_error("HttpAgent not initialized");
    }
    
    Request request;
    Candid candid(args);
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> _args = candid.encode();
    if(!_senderKeyPair.isInitialized()){
        request = Request(_canisterID, "query", method_name, _args);
        encoded = request.encode();
    } else {
        
        request = Request(_senderKeyPair.getPrincipal(), _canisterID, "query", method_name, _args);
        encoded = request.encode(_senderKeyPair);
    }

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
        
        String response = http.getString();

        // Assuming you get a CBOR response and want to print it as hex
        std::vector<uint8_t> responseBytes(response.begin(), response.end());
        for(auto byte : responseBytes) {
            printf("%02x", byte);
        }
        printf("\n");
        if(httpResponseCode == 200) {
            Response r = Response(responseBytes);
            Candid candidResponse(r.reply.arg);
            result = candidResponse.decode();
        } else {
            printf("Error on sending POST: ");
            printf("%d", httpResponseCode);
            printf("\n");
            printf("Error message: ");
            std::string responseString(responseBytes.begin(), responseBytes.end());
            printf(responseString.c_str());
            printf("\n");
        }
        
        
    } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        Serial.print("Error message: ");
        Serial.println(http.errorToString(httpResponseCode).c_str());
        throw std::runtime_error("Error on sending POST request");
    }
    http.end();      

    return result;
}
*/


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